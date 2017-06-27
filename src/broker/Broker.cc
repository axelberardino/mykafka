#include "broker/Broker.hh"
#include "utils/Utils.hh"

#include <set>
#include <memory>
#include <cassert>
#include <sstream>

namespace Broker
{
  const std::string CONFIG_FILENAME = "broker.conf";

  Broker::Broker(const std::string& base_path)
    : base_path_(base_path), config_manager_(base_path + "/config")
  {
  }

  Broker::~Broker()
  {
    close();
  }

  mykafka::Error
  Broker::createAndAddNewPartition(const std::string& path, const std::string& topic,
                                   int32_t partition_id, int64_t max_segment_size,
                                   int64_t max_partition_size, int64_t segment_ttl)
  {
    auto partition = std::make_shared<CommitLog::Partition>(path,
                                                            max_segment_size,
                                                            max_partition_size,
                                                            segment_ttl);
    auto res = partition->open();
    if (res.code() != mykafka::Error::OK)
      return res;

    auto& entry = topics_[{topic, partition_id}];
    entry.leader_id = 0; // Not used
    entry.preferred_leader_id = 0; // Not used
    entry.replicas.clear(); // Not used
    entry.isr.clear(); // Not used
    entry.partition = partition;

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Broker::load()
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);

    std::cout << "Loading all topics/partition from " << base_path_ << std::endl;
    auto res = config_manager_.load();
    if (res.code() != mykafka::Error::OK)
      return res;

    int32_t nb = 0;
    for (auto& cfg : config_manager_)
    {
      res = createAndAddNewPartition(base_path_ + "/" + cfg.first.toString(),
                                     cfg.first.topic, cfg.first.partition,
                                     cfg.second.info.max_segment_size,
                                     cfg.second.info.max_partition_size,
                                     cfg.second.info.segment_ttl);
      if (res.code() != mykafka::Error::OK)
        return res;
      ++nb;
    }
    std::cout << "Load " << nb << " partitions!" << std::endl;

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Broker::createPartition(mykafka::TopicPartitionRequest& request)
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);

    const std::string key = request.topic() + "-" + std::to_string(request.partition());
    auto found = topics_.find({request.topic(), request.partition()});
    if (found != topics_.cend())
      return Utils::err(mykafka::Error::TOPIC_ERROR,
                        "The topic/partition " + key + " already exists!");

    auto res = createAndAddNewPartition(base_path_ + "/" + key,
                                        request.topic(), request.partition(),
                                        request.max_segment_size(),
                                        request.max_partition_size(),
                                        request.segment_ttl());
    if (res.code() != mykafka::Error::OK)
      return res;

    return config_manager_.create({request.topic(), request.partition()},
                                  request.max_segment_size(),
                                  request.max_partition_size(),
                                  request.segment_ttl());
  }

  mykafka::Error
  Broker::deletePartition(mykafka::TopicPartitionRequest& request)
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);

    const std::string key = request.topic() + "-" + std::to_string(request.partition());
    auto found = topics_.find({request.topic(), request.partition()});
    if (found == topics_.cend())
      return Utils::err(mykafka::Error::TOPIC_ERROR,
                        "The topic " + key + " don't exists!");

    auto res = found->second.partition->deletePartition();
    if (res.code() != mykafka::Error::OK)
      return res;
    topics_.erase(found);

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Broker::deleteTopic(mykafka::TopicPartitionRequest& request)
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);

    std::vector<iterator> delete_list;

    auto end = topics_.end();
    for (auto entry = topics_.begin(); entry != end; ++entry)
    {
      if (entry->first.topic == request.topic())
      {
        auto res = entry->second.partition->deletePartition();
        if (res.code() != mykafka::Error::OK)
          return res;
        res = config_manager_.remove({request.topic(), entry->first.partition});
        if (res.code() != mykafka::Error::OK)
          return res;
        delete_list.push_back(entry);
      }
    }

    for (auto& entry : delete_list)
      topics_.erase(entry);

    return Utils::err(mykafka::Error::OK);
  }

  void
  Broker::getTopicInfo(mykafka::Void&,
                       mykafka::BrokerInfoResponse& response)
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    std::ostringstream buff;
    dump(buff);
    response.set_dump(buff.str());
    auto error = response.mutable_error();
    error->set_code(mykafka::Error::OK);
    error->set_msg("");
  }

  void
  Broker::getOffsets(mykafka::GetOffsetsRequest& request,
                     mykafka::GetOffsetsResponse& response)
  {
    const Utils::ConfigManager::TopicPartition key{request.topic(), request.partition()};
    auto error = response.mutable_error();
    {
      boost::shared_lock<boost::shared_mutex> lock(mutex_);

      const std::string strkey = request.topic() + "-" + std::to_string(request.partition());
      auto found = topics_.find(key);
      if (found == topics_.cend())
      {
        error->set_code(mykafka::Error::TOPIC_ERROR);
        error->set_msg("The topic " + strkey + " don't exists!");
        return;
      }

      response.set_first_offset(found->second.partition->oldestOffset());
      response.set_last_offset(found->second.partition->newestOffset() - 1);
    }

    Utils::ConfigManager::RawInfo info;
    auto res = config_manager_.get(key, info);
    error->set_code(res.code());
    error->set_msg(res.msg());

    if (res.code() == mykafka::Error::OK)
      response.set_commit_offset(info.commit_offset);
  }

  void
  Broker::getMessage(mykafka::GetMessageRequest& request,
                     mykafka::GetMessageResponse& response)
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);

    const Utils::ConfigManager::TopicPartition key{request.topic(), request.partition()};
    auto error = response.mutable_error();

    Utils::ConfigManager::RawInfo info;
    auto res = config_manager_.get(key, info);
    if (res.code() != mykafka::Error::OK)
    {
      error->set_code(res.code());
      error->set_msg(res.msg());
      return;
    }

    if (request.offset() > info.commit_offset)
    {
      error->set_code(mykafka::Error::NO_MESSAGE);
      error->set_msg("No more messages available!");
      return;
    }

    const std::string strkey = request.topic() + "-" + std::to_string(request.partition());
    auto found = topics_.find(key);
    if (found == topics_.cend())
    {
      error->set_code(mykafka::Error::TOPIC_ERROR);
      error->set_msg("The topic " + strkey + " don't exists!");
      return;
    }

    std::vector<char> payload;
    res = found->second.partition->readAt(payload, request.offset());
    error->set_code(res.code());
    error->set_msg(res.msg());
    response.set_payload(std::string(payload.begin(), payload.end()));
  }

  void
  Broker::sendMessage(mykafka::SendMessageRequest& request,
                      mykafka::SendMessageResponse& response)
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);

    const Utils::ConfigManager::TopicPartition key{request.topic(), request.partition()};
    auto error = response.mutable_error();
    int64_t offset = 0;
    {
      const std::string strkey = request.topic() + "-" + std::to_string(request.partition());
      auto found = topics_.find(key);
      if (found == topics_.cend())
      {
        error->set_code(mykafka::Error::TOPIC_ERROR);
        error->set_msg("The topic " + strkey + " don't exists!");
        return;
      }

      std::vector<char> payload(request.payload().begin(), request.payload().end());
      auto res = found->second.partition->write(payload, offset);
      error->set_code(res.code());
      error->set_msg(res.msg());
      if (res.code() != mykafka::Error::OK)
        return;
    }

    auto res = config_manager_.updateCommitOffset(key, offset);
    error->set_code(res.code());
    error->set_msg(res.msg());

    response.set_offset(offset);
  }

  mykafka::Error
  Broker::close()
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);

    for (auto& entry : topics_)
    {
      auto res = entry.second.partition->close();
      if (res.code() != mykafka::Error::OK)
        return res;
    }

    return config_manager_.close();
  }

  int32_t
  Broker::nbTopics() const
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    std::set<std::string> set;
    for (auto& entry : topics_)
      set.insert(entry.first.topic);
    return set.size();
  }

  int32_t
  Broker::nbPartitions() const
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    return topics_.size();
  }

  void
  Broker::dump(std::ostream& out) const
  {
    out << "== Topics/Partitions ==\n";
    for (auto& entry : topics_)
      out << entry.first.toString() << ": "
          << "leader: " << entry.second.leader_id
          << ", pref_leader:" << entry.second.preferred_leader_id
          << ", replicas: " << Utils::vecToStr(entry.second.replicas)
          << ", isr: " << Utils::vecToStr(entry.second.isr)
          << ", newestOffset: " << entry.second.partition->newestOffset()
          << ", oldestOffset: " << entry.second.partition->oldestOffset()
          << ", size: " << entry.second.partition->physicalSize()
          << std::endl;
    out << "== Config ==\n";
    config_manager_.dump(out);
  }
} // Broker

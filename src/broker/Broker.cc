#include "broker/Broker.hh"
#include "utils/Utils.hh"

#include <memory>
#include <cassert>

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
  Broker::load()
  {
    auto res = config_manager_.load();
    if (res.code() != mykafka::Error::OK)
      return res;

    for (auto& cfg : config_manager_)
    {
      const std::string part_path = base_path_ + "/" + cfg.first.toString();
      auto partition = std::make_shared<CommitLog::Partition>(part_path,
                                                              cfg.second.info.max_segment_size,
                                                              cfg.second.info.max_partition_size,
                                                              cfg.second.info.segment_ttl);
      res = partition->open();
      if (res.code() != mykafka::Error::OK)
        return res;

      auto& entry = topics_[cfg.first.topic][cfg.first.partition];
      entry.leader_id = 0; // FIXME
      entry.preferred_leader_id = 0; // FIXM
      entry.replicas.clear(); // FIXME
      entry.isr.clear(); // FIXME
      entry.partition = partition;
    }

    auto partition = std::make_shared<CommitLog::Partition>(base_path_ + "/default-0", 0, 0, 0);
    res = partition->open();
    if (res.code() != mykafka::Error::OK)
      return res;
    topics_["default"][0].partition = partition;

    return Utils::err(mykafka::Error::OK);
  }

  void
  Broker::getMessage(mykafka::GetMessageRequest& request,
                     mykafka::GetMessageResponse& response)
  {
    auto partition = topics_["default"][0].partition;
    assert(partition);

    std::cout << "Get message from topic " << request.topic()
              << " at offset: " << request.offset() << std::endl;

    std::vector<char> payload;
    auto res = partition->readAt(payload, request.offset());
    auto error = response.error();
    error.set_code(res.code());
    error.set_msg(res.msg());
    response.set_payload(std::string(payload.begin(), payload.end()));
  }

  void
  Broker::sendMessage(mykafka::SendMessageRequest& request,
                      mykafka::SendMessageResponse& response)
  {
    auto partition = topics_["default"][0].partition;
    assert(partition);

    std::cout << "Send to topic " << request.topic() << "-"
              << request.partition() << ": "
              << request.payload() << std::endl;

    std::vector<char> payload(request.payload().begin(), request.payload().end());
    int64_t offset = 0;
    auto res = partition->write(payload, offset);
    auto error = response.error();
    error.set_code(res.code());
    error.set_msg(res.msg());
    response.set_offset(offset);
  }

  mykafka::Error
  Broker::close()
  {
    for (auto& entry : topics_)
    {
      for (auto& partition : entry.second)
      {
        auto res = partition.second.partition->close();
        if (res.code() != mykafka::Error::OK)
          return res;
      }
    }

    return config_manager_.close();
  }
} // Broker

#ifndef BROKER_BROKER_HH_
# define BROKER_BROKER_HH_

# include "commitlog/Partition.hh"
# include "utils/ConfigManager.hh"

# include <boost/thread/shared_mutex.hpp>
# include <unordered_map>
# include <vector>
# include <inttypes.h>

namespace Broker
{
  /*!
  ** @class Broker
  **
  ** This class hold a list of partition
  ** with their information.
  **
  ** Topic:bookstore
  **         partition 0:
  **             commitLog: []
  **             first_offset: 500
  **             next_offset: 456672
  **             commit_offset: 456652
  **             is_leader: true
  **             followers: [localhost:9002,localhost:9003]
  **         partition 1:
  **             commitLog: []
  **             first_offset: 600
  **             next_offset: 456542
  **             commit_offset: 456438
  **             is_leader: false
  **             followers: []
  ** Topic:events
  **         etc...
  */
  class Broker
  {
  public:
    Broker(const std::string& base_path);
    ~Broker();

    /*!
    ** Load all config files, then all partitions
    ** present in the config files.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error load();

    /*!
    ** Create a partition on the given topic.
    ** @warning Will failed if the partition already exists!
    **
    ** @param request The client request
    **        (needed: topic, partition, max_segment_size,
    **        max_partition_size, segment_ttl).
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error createPartition(mykafka::TopicPartitionRequest& request);

    /*!
    ** Create a partition on the given topic.
    ** @warning Will failed if the partition don't exists!
    **
    ** @param request The client request
    **        (needed: topic, partition).
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error deletePartition(mykafka::TopicPartitionRequest& request);

    /*!
    ** Delete an entire topic and all its partition.
    **
    ** @param request The client request.
    **        (needed: topic).
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error deleteTopic(mykafka::TopicPartitionRequest& request);

    /*!
    ** Dump broker info.
    **
    ** @param request The client request.
    ** @param response The response to give to the client.
    */
    void getTopicInfo(mykafka::Void&,
                      mykafka::BrokerInfoResponse& response);

    /*!
    ** Get low offset, commit offset and high offset.
    **
    ** @param request The client request.
    ** @param response The response to give to the client.
    */
    void getOffsets(mykafka::GetOffsetsRequest& request,
                    mykafka::GetOffsetsResponse& response);

    /*!
    ** Get a message from the selected topic/partition.
    ** If topic/partition not exists, an error will be
    ** filled into the response.
    **
    ** @param request The client request.
    ** @param response The response to give to the client.
    */
    void getMessage(mykafka::GetMessageRequest& request,
                    mykafka::GetMessageResponse& response);

    /*!
    ** Write a message to the selected topic/partition.
    ** If topic/partition not exists, an error will be
    ** filled into the response.
    **
    ** @param request The client request.
    ** @param response The response to give to the client.
    */
    void sendMessage(mykafka::SendMessageRequest& request,
                     mykafka::SendMessageResponse& response);

    /*!
    ** Close all partition and config files.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error close();

    /*!
    ** Get the number of topics hold.
    **
    ** @return The number of topics.
    */
    int32_t nbTopics() const;

    /*!
    ** Get the numer of partitions hold.
    **
    ** @return The number of partitions.
    */
    int32_t nbPartitions() const;

    /*!
    ** Dump the entire broker info into a stream.
    **
    ** @param out The output stream.
    */
    void dump(std::ostream& out) const;

  private:
    /*!
    ** Create a new partition, and add it to the topics list.
    **
    ** @param path The physical path of the partition.
    ** @param topic The topic name.
    ** @param partition_id The partition number.
    ** @param max_segment_size Max size per segment.
    ** @param max_partition_size Max total partition size allowed.
    ** @param segment_ttl Life duration of a segment.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error createAndAddNewPartition(const std::string& path,  const std::string& topic,
                                            int32_t partition_id, int64_t max_segment_size,
                                            int64_t max_partition_size, int64_t segment_ttl);

  private:
    struct PartitionInfo
    {
      int32_t leader_id;
      int32_t preferred_leader_id;
      std::vector<std::string> replicas;
      std::vector<std::string> isr;
      std::shared_ptr<CommitLog::Partition> partition;
    };
    typedef std::unordered_map<Utils::ConfigManager::TopicPartition,
                               PartitionInfo,
                               Utils::Hash<Utils::ConfigManager::TopicPartition> > topics_type;
    typedef topics_type::iterator iterator;

  private:
    const std::string base_path_;
    topics_type topics_;
    Utils::ConfigManager config_manager_;
    mutable boost::shared_mutex mutex_;
  };
} // Broker

#endif /* !BROKER_BROKER_HH_ */

#ifndef BROKER_BROKER_HH_
# define BROKER_BROKER_HH_

# include "commitlog/Partition.hh"
# include "utils/ConfigManager.hh"

# include <map>
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

    void getMessage(mykafka::GetMessageRequest& request,
                    mykafka::GetMessageResponse& response);

    void sendMessage(mykafka::SendMessageRequest& request,
                     mykafka::SendMessageResponse& response);

    mykafka::Error load();
    mykafka::Error createTopic();
    mykafka::Error deleteTopic();
    mykafka::Error getTopicInfo();

    /*!
    ** Close all partition and config files.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error close();

    /*!
    ** Dump the entire broker info into a stream.
    **
    ** @param out The output stream.
    */
    void dump(std::ostream& out) const;

  private:
    const std::string base_path_;

    struct PartitionInfo
    {
      int32_t leader_id;
      int32_t preferred_leader_id;
      std::vector<std::string> replicas;
      std::vector<std::string> isr;
      std::shared_ptr<CommitLog::Partition> partition;
    };
    std::map<std::string, std::map<int32_t, PartitionInfo> > topics_;
    Utils::ConfigManager config_manager_;
  };
} // Broker

#endif /* !BROKER_BROKER_HH_ */

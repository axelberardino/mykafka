#ifndef BROKER_BROKER_HH_
# define BROKER_BROKER_HH_

# include "commitlog/Partition.hh"

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
    Broker();
    ~Broker();

    void getMessage(mykafka::GetMessageRequest& request,
                    mykafka::GetMessageResponse& response);

    void sendMessage(mykafka::SendMessageRequest& request,
                     mykafka::SendMessageResponse& response);

  private:
    struct Info
    {
      int64_t first_offset;
      int64_t next_offset;
      int64_t commit_offset;
      bool is_leader;
      std::vector<std::string> followers;
      CommitLog::Partition partition;
    };
    std::map<std::string, std::map<int32_t, Info> > topics_;
  };
} // Broker

#endif /* !BROKER_BROKER_HH_ */

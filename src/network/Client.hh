#ifndef NETWORK_CLIENT_HH_
# define NETWORK_CLIENT_HH_

# include <grpc++/grpc++.h>
# include <grpc/support/log.h>

# include "mykafka.grpc.pb.h"

# include <memory>

namespace Network
{
  class Client
  {
  public:
    Client(std::string address, int64_t client_connection_timeout = 200 /* ms */);
    bool reconnect();
    grpc::Status sendMessage(mykafka::SendMessageRequest request,
                             mykafka::SendMessageResponse& response);

  private:
    const std::string address_;
    int64_t client_connection_timeout_;
    int64_t reconnect_timeout_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<mykafka::Broker::Stub> stub_;
  };
} // Network

#endif /* !NETWORK_CLIENT_HH_ */

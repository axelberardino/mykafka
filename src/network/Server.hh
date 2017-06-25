#ifndef NETWORK_SERVER_HH_
# define NETWORK_SERVER_HH_

# include <grpc++/grpc++.h>
# include <grpc/support/log.h>

# include "mykafka.grpc.pb.h"

# include <memory>

namespace Network
{
  class Server
  {
  public:
    Server(std::string address);
    ~Server();
    void run();
    void handleRpcs();

  private:
    bool started_;
    const std::string address_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    mykafka::Broker::AsyncService service_;
    std::unique_ptr<grpc::Server> server_;
  };
} // Network

#endif /* !NETWORK_SERVER_HH_ */

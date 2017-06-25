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
    Server();
    ~Server();
    void run();
    void handleRpcs();

  private:
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    mykafka::Broker::AsyncService service_;
    std::unique_ptr<grpc::Server> server_;
  };
} // Network

#endif /* !NETWORK_SERVER_HH_ */

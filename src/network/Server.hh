#ifndef NETWORK_SERVER_HH_
# define NETWORK_SERVER_HH_

# include <grpc++/grpc++.h>
# include <grpc/support/log.h>

# include "mykafka.grpc.pb.h"

# include <memory>
# include <thread>

namespace Network
{
  /*!
  ** @class Server
  **
  ** Class use to ease the write of myKafka servers.
  */
  class Server
  {
  public:
    /*!
    ** Intialize a server.
    **
    ** @param address The server + port (server:port)
    */
    Server(std::string address,
           int32_t thread_number = std::thread::hardware_concurrency());

    /*!
    ** Shutdown grpc server and completion queue.
    */
    ~Server();

    /*!
    ** Launch the server and start to listen.
    */
    void run();

  protected:
    /*!
    ** Handle all services.
    */
    void handleRpcs();

  private:
    bool started_;
    int32_t thread_number_;
    const std::string address_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    mykafka::Broker::AsyncService service_;
    std::unique_ptr<grpc::Server> server_;
  };
} // Network

#endif /* !NETWORK_SERVER_HH_ */

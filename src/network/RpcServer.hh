#ifndef NETWORK_SERVER_HH_
# define NETWORK_SERVER_HH_

# include <grpc++/grpc++.h>
# include <grpc/support/log.h>

# include "mykafka.grpc.pb.h"

# include <memory>

namespace Network
{
  /*!
  ** @class RpcServer
  **
  ** Class use to ease the write of myKafka servers.
  */
  class RpcServer
  {
  public:
    /*!
    ** Intialize a server.
    **
    ** @param address The server + port (server:port)
    ** @param service Register the given service.
    ** @param thread_number Number of working thread (0 = nb machine core).
    */
    RpcServer(std::string address,
              std::shared_ptr<grpc::Service> service,
              int32_t thread_number = 0);

    /*!
    ** Shutdown grpc server and completion queue.
    */
    virtual ~RpcServer();

    /*!
    ** Launch the server and start to listen.
    */
    void run();

  protected:
    /*!
    ** Handle all services.
    */
    void handleRpcs();

    /*!
    ** Handle specific method for rpc server
    */
    virtual void specificHandle() = 0;

  protected:
    bool started_;
    int32_t thread_number_;
    const std::string address_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    std::shared_ptr<grpc::Service> service_;
    std::unique_ptr<grpc::Server> server_;
  };
} // Network

#endif /* !NETWORK_SERVER_HH_ */

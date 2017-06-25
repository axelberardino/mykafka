#ifndef NETWORK_RPCSERVICE_HH_
# define NETWORK_RPCSERVICE_HH_

# include <grpc++/grpc++.h>
# include <grpc/support/log.h>

# include "mykafka.grpc.pb.h"

namespace Network
{
  /*!
  ** @class Service
  **
  ** Abstract class use to handle rpc service.
  */
  class RpcService
  {
  public:
    /*!
    ** Initialize a new service.
    **
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    RpcService(std::shared_ptr<grpc::Service> service, grpc::ServerCompletionQueue* cq);

    /*!
    ** Destroy the service.
    */
    virtual ~RpcService();

    /*!
    ** Spawn a new Service instance to serve new clients.
    ** This instance will deallocate itself as
    ** part of its FINISH state.
    */
    void proceed();

  protected:
    /*!
    ** Main method. Has to be override to handle works.
    */
    virtual void process() = 0;

  protected:
    std::shared_ptr<grpc::Service> service_;
    grpc::ServerCompletionQueue* cq_;

  private:
    enum CallStatus { PROCESS, FINISH };
    CallStatus status_;
  };
} // Network

#endif /* !NETWORK_RPCSERVICE_HH_ */

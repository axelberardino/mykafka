#ifndef NETWORK_SERVICE_HH_
# define NETWORK_SERVICE_HH_

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
  class Service
  {
  public:
    /*!
    ** Initialize a new service.
    **
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    Service(mykafka::Broker::AsyncService* service, grpc::ServerCompletionQueue* cq);

    /*!
    ** Destroy the service.
    */
    virtual ~Service();

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
    mykafka::Broker::AsyncService* service_;
    grpc::ServerCompletionQueue* cq_;

  private:
    enum CallStatus { PROCESS, FINISH };
    CallStatus status_;
  };
} // Network

#endif /* !NETWORK_SERVICE_HH_ */

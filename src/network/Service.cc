#include "network/Service.hh"

namespace Network
{
  Service::Service(mykafka::Broker::AsyncService* service,
                   grpc::ServerCompletionQueue* cq)
      : service_(service), cq_(cq), status_(PROCESS)
  {
  }

  Service::~Service()
  {
  }

  void
  Service::proceed()
  {
    if (status_ == PROCESS)
    {
      status_ = FINISH;
      process();
    }
    else
    {
      GPR_ASSERT(status_ == FINISH);
      delete this;
    }
  }
} // Network

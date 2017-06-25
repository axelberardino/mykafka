#include "network/RpcService.hh"

namespace Network
{
  RpcService::RpcService(std::shared_ptr<grpc::Service> service,
                         grpc::ServerCompletionQueue* cq)
    : service_(service), cq_(cq), status_(PROCESS)
  {
  }

  RpcService::~RpcService()
  {
  }

  void
  RpcService::proceed()
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

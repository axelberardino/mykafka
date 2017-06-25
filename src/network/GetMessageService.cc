#include "GetMessageService.hh"

namespace Network
{
  GetMessageService::GetMessageService(Broker::Broker& broker,
                                       std::shared_ptr<grpc::Service> service,
                                       grpc::ServerCompletionQueue* cq)
    : RpcService(service, cq), responder_(&ctx_), broker_(broker)
  {
    auto async_service = static_cast<mykafka::Broker::AsyncService*>(service.get());
    async_service->RequestGetMessage(&ctx_, &request_, &responder_, cq, cq, this);
  }

  GetMessageService::~GetMessageService()
  {
  }

  void
  GetMessageService::process()
  {
    new GetMessageService(broker_, service_, cq_);
    broker_.getMessage(request_, response_);
    responder_.Finish(response_, grpc::Status::OK, this);
  }
} // Network

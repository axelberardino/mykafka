#include "GetMessageService.hh"

namespace Network
{
  GetMessageService::GetMessageService(std::shared_ptr<grpc::Service> service,
                                       grpc::ServerCompletionQueue* cq)
    : Service(service, cq), responder_(&ctx_)
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
    new GetMessageService(service_, cq_);

    std::cout << "Get message at offset: " << request_.offset() << std::endl;
    auto error = response_.error();
    error.set_code(mykafka::Error::OK);
    error.set_msg("ok");
    response_.set_payload("Got payload");

    responder_.Finish(response_, grpc::Status::OK, this);
  }
} // Network

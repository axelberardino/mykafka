#include "SendMessageService.hh"

namespace Network
{
  SendMessageService::SendMessageService(std::shared_ptr<grpc::Service> service,
                                         grpc::ServerCompletionQueue* cq)
    : Service(service, cq), responder_(&ctx_)
  {
    auto async_service = static_cast<mykafka::Broker::AsyncService*>(service.get());
    async_service->RequestSendMessage(&ctx_, &request_, &responder_, cq, cq, this);
  }

  SendMessageService::~SendMessageService()
  {
  }

  void
  SendMessageService::process()
  {
    new SendMessageService(service_, cq_);

    std::cout << "Call commit log: " << request_.payload() << std::endl;
    auto error = response_.error();
    error.set_code(mykafka::Error::OK);
    error.set_msg("ok");
    response_.set_offset(-1);

    responder_.Finish(response_, grpc::Status::OK, this);
  }
} // Network

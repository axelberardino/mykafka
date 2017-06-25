#include "SendMessageService.hh"

namespace Network
{
  SendMessageService::SendMessageService(Broker::Broker& broker,
                                         std::shared_ptr<grpc::Service> service,
                                         grpc::ServerCompletionQueue* cq)
    : RpcService(service, cq), responder_(&ctx_), broker_(broker)
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
    new SendMessageService(broker_, service_, cq_);

    std::cout << "Call commit log: " << request_.payload() << std::endl;
    auto error = response_.error();
    error.set_code(mykafka::Error::OK);
    error.set_msg("ok");
    response_.set_offset(-1);

    responder_.Finish(response_, grpc::Status::OK, this);
  }
} // Network

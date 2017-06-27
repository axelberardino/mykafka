#include "network/GetOffsetsService.hh"

namespace Network
{
  GetOffsetsService::GetOffsetsService(Broker::Broker& broker,
                                       std::shared_ptr<grpc::Service> service,
                                       grpc::ServerCompletionQueue* cq)
    : RpcService(service, cq), responder_(&ctx_), broker_(broker)
  {
    auto async_service = static_cast<mykafka::Broker::AsyncService*>(service.get());
    async_service->RequestGetOffsets(&ctx_, &request_, &responder_, cq, cq, this);
  }

  GetOffsetsService::~GetOffsetsService()
  {
  }

  void
  GetOffsetsService::process()
  {
    new GetOffsetsService(broker_, service_, cq_);
    broker_.getOffsets(request_, response_);
    responder_.Finish(response_, grpc::Status::OK, this);
  }
} // Network

#include "network/DeleteTopicService.hh"

namespace Network
{
  DeleteTopicService::DeleteTopicService(Broker::Broker& broker,
                                         std::shared_ptr<grpc::Service> service,
                                         grpc::ServerCompletionQueue* cq)
    : RpcService(service, cq), responder_(&ctx_), broker_(broker)
  {
    auto async_service = static_cast<mykafka::Broker::AsyncService*>(service.get());
    async_service->RequestDeleteTopic(&ctx_, &request_, &responder_, cq, cq, this);
  }

  DeleteTopicService::~DeleteTopicService()
  {
  }

  void
  DeleteTopicService::process()
  {
    new DeleteTopicService(broker_, service_, cq_);
    std::cout << "Ask to delete topic: " << request_.topic() << std::endl;
    response_ = broker_.deleteTopic(request_);
    responder_.Finish(response_, grpc::Status::OK, this);
  }
} // Network

#include "network/BrokerInfoService.hh"

namespace Network
{
  BrokerInfoService::BrokerInfoService(Broker::Broker& broker,
                                       std::shared_ptr<grpc::Service> service,
                                       grpc::ServerCompletionQueue* cq)
    : RpcService(service, cq), responder_(&ctx_), broker_(broker)
  {
    auto async_service = static_cast<mykafka::Broker::AsyncService*>(service.get());
    async_service->RequestBrokerInfo(&ctx_, &request_, &responder_, cq, cq, this);
  }

  BrokerInfoService::~BrokerInfoService()
  {
  }

  void
  BrokerInfoService::process()
  {
    new BrokerInfoService(broker_, service_, cq_);
    std::cout << "Ask for broker info" << std::endl;
    broker_.getTopicInfo(request_, response_);
    responder_.Finish(response_, grpc::Status::OK, this);
  }
} // Network

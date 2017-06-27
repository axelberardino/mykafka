#include "network/CreatePartitionService.hh"

namespace Network
{
  CreatePartitionService::CreatePartitionService(Broker::Broker& broker,
                                                 std::shared_ptr<grpc::Service> service,
                                                 grpc::ServerCompletionQueue* cq)
    : RpcService(service, cq), responder_(&ctx_), broker_(broker)
  {
    auto async_service = static_cast<mykafka::Broker::AsyncService*>(service.get());
    async_service->RequestCreatePartition(&ctx_, &request_, &responder_, cq, cq, this);
  }

  CreatePartitionService::~CreatePartitionService()
  {
  }

  void
  CreatePartitionService::process()
  {
    new CreatePartitionService(broker_, service_, cq_);
    std::cout << "Ask to create topic/partition: "
              << request_.topic() << "/" << request_.partition() << std::endl;
    response_ = broker_.createPartition(request_);
    responder_.Finish(response_, grpc::Status::OK, this);
  }
} // Network

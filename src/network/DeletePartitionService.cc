#include "network/DeletePartitionService.hh"

namespace Network
{
  DeletePartitionService::DeletePartitionService(Broker::Broker& broker,
                                                 std::shared_ptr<grpc::Service> service,
                                                 grpc::ServerCompletionQueue* cq)
    : RpcService(service, cq), responder_(&ctx_), broker_(broker)
  {
    auto async_service = static_cast<mykafka::Broker::AsyncService*>(service.get());
    async_service->RequestDeletePartition(&ctx_, &request_, &responder_, cq, cq, this);
  }

  DeletePartitionService::~DeletePartitionService()
  {
  }

  void
  DeletePartitionService::process()
  {
    new DeletePartitionService(broker_, service_, cq_);
    std::cout << "Ask to delete topic/partition: "
              << request_.topic() << "/" << request_.partition() << std::endl;
    response_ = broker_.deletePartition(request_);
    responder_.Finish(response_, grpc::Status::OK, this);
  }
} // Network

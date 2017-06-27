#ifndef NETWORK_CREATEPARTITIONSERVICE_HH_
# define NETWORK_CREATEPARTITIONSERVICE_HH_

# include "network/RpcService.hh"
# include "broker/Broker.hh"

namespace Network
{
  /*!
  ** @class CreatePartitionService
  **
  ** Handle create partition.
  */
  class CreatePartitionService : public RpcService
  {
  public:
    /*!
    ** Initialize a get message service.
    **
    ** @param broker The broker.
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    CreatePartitionService(Broker::Broker& broker,
                           std::shared_ptr<grpc::Service> service,
                           grpc::ServerCompletionQueue* cq);

    /*!
    ** Destroy the service.
    */
    virtual ~CreatePartitionService();

    /*!
    ** Handle the service.
    ** Get a payload to the commitlog, and returns the offset.
    */
    void process() override;

  private:
    grpc::ServerContext ctx_;
    mykafka::TopicPartitionRequest request_;
    mykafka::Error response_;
    grpc::ServerAsyncResponseWriter<mykafka::Error> responder_;
    Broker::Broker& broker_;
  };
} // Network

#endif /* !NETWORK_CREATEPARTITIONSERVICE_HH_ */

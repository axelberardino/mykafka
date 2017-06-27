#ifndef NETWORK_DELETEPARTITIONSERVICE_HH_
# define NETWORK_DELETEPARTITIONSERVICE_HH_

# include "network/RpcService.hh"
# include "broker/Broker.hh"

namespace Network
{
  /*!
  ** @class DeletePartitionService
  **
  ** Handle get messsage.
  */
  class DeletePartitionService : public RpcService
  {
  public:
    /*!
    ** Initialize a get message service.
    **
    ** @param broker The broker.
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    DeletePartitionService(Broker::Broker& broker,
                      std::shared_ptr<grpc::Service> service,
                      grpc::ServerCompletionQueue* cq);

    /*!
    ** Destroy the service.
    */
    virtual ~DeletePartitionService();

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

#endif /* !NETWORK_DELETEPARTITIONSERVICE_HH_ */

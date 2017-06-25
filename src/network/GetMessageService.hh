#ifndef NETWORK_GETMESSAGESERVICE_HH_
# define NETWORK_GETMESSAGESERVICE_HH_

# include "network/RpcService.hh"
# include "broker/Broker.hh"

namespace Network
{
  /*!
  ** @class GetMessageService
  **
  ** Handle get messsage.
  */
  class GetMessageService : public RpcService
  {
  public:
    /*!
    ** Initialize a get message service.
    **
    ** @param broker The broker.
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    GetMessageService(Broker::Broker& broker,
                      std::shared_ptr<grpc::Service> service,
                      grpc::ServerCompletionQueue* cq);

    /*!
    ** Destroy the service.
    */
    virtual ~GetMessageService();

    /*!
    ** Handle the service.
    ** Get a payload to the commitlog, and returns the offset.
    */
    void process() override;

  private:
    grpc::ServerContext ctx_;
    mykafka::GetMessageRequest request_;
    mykafka::GetMessageResponse response_;
    grpc::ServerAsyncResponseWriter<mykafka::GetMessageResponse> responder_;
    Broker::Broker& broker_;
  };
} // Network

#endif /* !NETWORK_GETMESSAGESERVICE_HH_ */

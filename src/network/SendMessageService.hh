#ifndef NETWORK_SENDMESSAGESERVICE_HH_
# define NETWORK_SENDMESSAGESERVICE_HH_

# include "network/RpcService.hh"

namespace Network
{
  /*!
  ** @class SendMessageService
  **
  ** Handle send messsage.
  */
  class SendMessageService : public RpcService
  {
  public:
    /*!
    ** Initialize a send message service.
    **
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    SendMessageService(std::shared_ptr<grpc::Service> service,
                       grpc::ServerCompletionQueue* cq);

    /*!
    ** Destroy the service.
    */
    virtual ~SendMessageService();

    /*!
    ** Handle the service.
    ** Send a payload to the commitlog, and returns the offset.
    */
    void process() override;

  private:
    grpc::ServerContext ctx_;
    mykafka::SendMessageRequest request_;
    mykafka::SendMessageResponse response_;
    grpc::ServerAsyncResponseWriter<mykafka::SendMessageResponse> responder_;
  };
} // Network

#endif /* !NETWORK_SENDMESSAGESERVICE_HH_ */

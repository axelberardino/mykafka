#ifndef NETWORK_SENDMESSAGESERVICE_HH_
# define NETWORK_SENDMESSAGESERVICE_HH_

# include "network/Service.hh"

namespace Network
{
  /*!
  ** @class SendMessageService
  **
  ** Handle send messsage.
  */
  class SendMessageService : public Service
  {
  public:
    /*!
    ** Initialize a send message service.
    **
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    SendMessageService(mykafka::Broker::AsyncService* service,
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

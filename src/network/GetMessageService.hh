#ifndef NETWORK_GETMESSAGESERVICE_HH_
# define NETWORK_GETMESSAGESERVICE_HH_

# include "network/Service.hh"

namespace Network
{
  /*!
  ** @class GetMessageService
  **
  ** Handle get messsage.
  */
  class GetMessageService : public Service
  {
  public:
    /*!
    ** Initialize a get message service.
    **
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    GetMessageService(mykafka::Broker::AsyncService* service,
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
  };
} // Network

#endif /* !NETWORK_GETMESSAGESERVICE_HH_ */

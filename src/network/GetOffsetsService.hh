#ifndef NETWORK_GETOFFSETSSERVICE_HH_
# define NETWORK_GETOFFSETSSERVICE_HH_

# include "network/RpcService.hh"
# include "broker/Broker.hh"

namespace Network
{
  /*!
  ** @class GetOffsetsService
  **
  ** Handle get offsets messsage.
  */
  class GetOffsetsService : public RpcService
  {
  public:
    /*!
    ** Initialize a get offsets message service.
    **
    ** @param broker The broker.
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    GetOffsetsService(Broker::Broker& broker,
                      std::shared_ptr<grpc::Service> service,
                      grpc::ServerCompletionQueue* cq);

    /*!
    ** Destroy the service.
    */
    virtual ~GetOffsetsService();

    /*!
    ** Handle the service.
    ** Send a payload to the commitlog, and returns the offset.
    */
    void process() override;

  private:
    grpc::ServerContext ctx_;
    mykafka::GetOffsetsRequest request_;
    mykafka::GetOffsetsResponse response_;
    grpc::ServerAsyncResponseWriter<mykafka::GetOffsetsResponse> responder_;
    Broker::Broker& broker_;
  };
} // Network

#endif /* !NETWORK_GETOFFSETSSERVICE_HH_ */

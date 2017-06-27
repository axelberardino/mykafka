#ifndef NETWORK_BROKERINFOSERVICE_HH_
# define NETWORK_BROKERINFOSERVICE_HH_

# include "network/RpcService.hh"
# include "broker/Broker.hh"

namespace Network
{
  /*!
  ** @class BrokerInfoService
  **
  ** Handle broker info messsage.
  */
  class BrokerInfoService : public RpcService
  {
  public:
    /*!
    ** Initialize a broker info service.
    **
    ** @param broker The broker.
    ** @param service The rpc async service.
    ** @param cq The async completion queue.
    */
    BrokerInfoService(Broker::Broker& broker,
                      std::shared_ptr<grpc::Service> service,
                      grpc::ServerCompletionQueue* cq);

    /*!
    ** Destroy the service.
    */
    virtual ~BrokerInfoService();

    /*!
    ** Handle the service.
    ** Send a payload to the commitlog, and returns the offset.
    */
    void process() override;

  private:
    grpc::ServerContext ctx_;
    mykafka::Void request_;
    mykafka::BrokerInfoResponse response_;
    grpc::ServerAsyncResponseWriter<mykafka::BrokerInfoResponse> responder_;
    Broker::Broker& broker_;
  };
} // Network

#endif /* !NETWORK_BROKERINFOSERVICE_HH_ */

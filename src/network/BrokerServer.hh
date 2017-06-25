#ifndef NETWORK_BROKERSERVER_HH_
# define NETWORK_BROKERSERVER_HH_

# include "network/RpcServer.hh"
# include "broker/Broker.hh"

# include <memory>

namespace Network
{
  /*!
  ** @class BrokerServer
  **
  ** Transform the broker into a rpc service.
  */
  class BrokerServer : public RpcServer
  {
  public:
    /*!
    ** Intialize a broker server.
    **
    ** @param address The server + port (server:port)
    ** @param service Register the given service.
    ** @param thread_number Number of working thread (0 = nb machine core).
    */
    BrokerServer(std::string address, Broker::Broker& broker, int32_t thread_number);

    /*!
    ** Shutdown grpc server and completion queue.
    */
    virtual ~BrokerServer();

  protected:
    /*!
    ** Handle all services.
    */
    void specificHandle() override;

  private:
    Broker::Broker& broker_;
  };
} // Network

#endif /* !NETWORK_BROKERSERVER_HH_ */

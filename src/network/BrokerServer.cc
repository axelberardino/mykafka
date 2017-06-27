#include "network/BrokerServer.hh"
#include "network/SendMessageService.hh"
#include "network/GetMessageService.hh"
#include "network/GetOffsetsService.hh"
#include "network/BrokerInfoService.hh"

#include <thread>
#include <vector>

namespace Network
{
  BrokerServer::BrokerServer(std::string address,
                             Broker::Broker& broker, int32_t thread_number)
    : RpcServer(address, std::make_shared<mykafka::Broker::AsyncService>(),
                thread_number), broker_(broker)
  {
  }

  BrokerServer::~BrokerServer()
  {
  }

  void
  BrokerServer::specificHandle()
  {
    new SendMessageService(broker_, service_, cq_.get());
    new GetMessageService(broker_, service_, cq_.get());
    new GetOffsetsService(broker_, service_, cq_.get());
    new BrokerInfoService(broker_, service_, cq_.get());
  }
} // Network

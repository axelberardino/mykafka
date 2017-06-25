#include "broker/Broker.hh"

namespace Broker
{
  Broker::Broker()
  {
  }

  Broker::~Broker()
  {
  }

  void
  Broker::getMessage(mykafka::GetMessageRequest& request,
                     mykafka::GetMessageResponse& response)
  {
    std::cout << "Get message at offset: " << request.offset() << std::endl;
    auto error = response.error();
    error.set_code(mykafka::Error::OK);
    error.set_msg("ok");
    response.set_payload("Got payload");
  }

  void
  Broker::sendMessage(mykafka::SendMessageRequest& request,
                      mykafka::SendMessageResponse& response)
  {
    std::cout << "Call commit log: " << request.payload() << std::endl;
    auto error = response.error();
    error.set_code(mykafka::Error::OK);
    error.set_msg("ok");
    response.set_offset(-1);
  }
} // Broker

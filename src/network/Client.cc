#include "network/Client.hh"

#include <chrono>

namespace Network
{
  Client::Client(std::string address, int64_t client_connection_timeout)
    : address_(address),
      client_connection_timeout_(client_connection_timeout),
      reconnect_timeout_(60000 /* 1 min */),
      channel_(grpc::CreateChannel(address, grpc::InsecureChannelCredentials())),
      stub_(mykafka::Broker::NewStub(channel_))
  {
  }

  bool
  Client::reconnect()
  {
    return channel_->WaitForConnected(std::chrono::system_clock::now() +
                                      std::chrono::milliseconds(reconnect_timeout_));
  }

  grpc::Status
  Client::sendMessage(mykafka::SendMessageRequest request,
                      mykafka::SendMessageResponse& response)
  {
    grpc::ClientContext context;
    if (client_connection_timeout_ > 0)
      context.set_deadline(std::chrono::system_clock::now() +
                           std::chrono::milliseconds(client_connection_timeout_));

    grpc::Status status = stub_->SendMessage(&context, request, &response);
    if (!status.ok())
    {
      std::cout << "Connection lost, try to reconnect..." << std::endl;
      const bool res = reconnect();
      std::cout << "Reconnect " << (res ? "succeed!" : "failed!") << std::endl;
    }

    return status;
  }
} // Network

#include "network/Client.hh"

#include <chrono>

namespace Network
{
  Client::Client(std::string address, int64_t client_connection_timeout)
    : address_(address),
      client_connection_timeout_(),
      channel_(grpc::CreateChannel(address, grpc::InsecureChannelCredentials())),
      stub_(mykafka::Broker::NewStub(channel_))
  {
  }

  void
  Client::reconnect()
  {
    // Try to reconnect
    channel_->GetState(true);
    channel_ = grpc::CreateChannel(address_, grpc::InsecureChannelCredentials());
    stub_ = mykafka::Broker::NewStub(channel_);
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
      //reconnect(); // Reconnect ?
    }

    return status;
  }
} // Network

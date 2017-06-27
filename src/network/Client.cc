#include "network/Client.hh"

#include <chrono>

#define METHOD_IMPL(SERVICE)                                            \
  do {                                                                  \
    grpc::ClientContext context;                                        \
    if (client_connection_timeout_ > 0)                                 \
      context.set_deadline(std::chrono::system_clock::now() +           \
                           std::chrono::milliseconds(client_connection_timeout_)); \
                                                                        \
    grpc::Status status = stub_->SERVICE(&context, request, &response); \
    while (try_reconnect && !status.ok())                               \
    {                                                                   \
      std::cout << "Connection lost, try to reconnect..." << std::endl; \
      const bool res = reconnect();                                     \
      std::cout << "Reconnect " << (res ? "succeed!" : "failed!") << std::endl; \
      grpc::ClientContext context;                                      \
      if (client_connection_timeout_ > 0)                               \
        context.set_deadline(std::chrono::system_clock::now() +         \
                             std::chrono::milliseconds(client_connection_timeout_)); \
      status = stub_->SERVICE(&context, request, &response);            \
    }                                                                   \
                                                                        \
    return status;                                                      \
  } while (0)


namespace Network
{
  Client::Client(std::string address, int64_t client_connection_timeout)
    : address_(address),
      client_connection_timeout_(client_connection_timeout),
      reconnect_timeout_(30 * 1000 /* 30 sec */),
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
  Client::sendMessage(mykafka::SendMessageRequest& request,
                      mykafka::SendMessageResponse& response,
                      bool try_reconnect)
  {
    METHOD_IMPL(SendMessage);
  }

  grpc::Status
  Client::getMessage(mykafka::GetMessageRequest& request,
                     mykafka::GetMessageResponse& response,
                     bool try_reconnect)
  {
    METHOD_IMPL(GetMessage);
  }

  grpc::Status
  Client::createPartition(mykafka::TopicPartitionRequest& request,
                          mykafka::Error& response,
                          bool try_reconnect)
  {
    METHOD_IMPL(CreatePartition);
  }

  grpc::Status
  Client::deletePartition(mykafka::TopicPartitionRequest& request,
                          mykafka::Error& response,
                          bool try_reconnect)
  {
    METHOD_IMPL(DeletePartition);
  }

  grpc::Status
  Client::deleteTopic(mykafka::TopicPartitionRequest& request,
                      mykafka::Error& response,
                      bool try_reconnect)
  {
    METHOD_IMPL(DeleteTopic);
  }

  grpc::Status
  Client::getOffsets(mykafka::GetOffsetsRequest& request,
                     mykafka::GetOffsetsResponse& response,
                     bool try_reconnect)
  {
    METHOD_IMPL(GetOffsets);
  }

  grpc::Status
  Client::brokerInfo(mykafka::Void& request,
                     mykafka::BrokerInfoResponse& response,
                     bool try_reconnect)
  {
    METHOD_IMPL(BrokerInfo);
  }
} // Network

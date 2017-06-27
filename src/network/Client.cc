#include "network/Client.hh"

#include <chrono>

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

  grpc::Status
  Client::getMessage(mykafka::GetMessageRequest& request,
                     mykafka::GetMessageResponse& response)
  {
    grpc::ClientContext context;
    if (client_connection_timeout_ > 0)
      context.set_deadline(std::chrono::system_clock::now() +
                           std::chrono::milliseconds(client_connection_timeout_));

    grpc::Status status = stub_->GetMessage(&context, request, &response);
    if (!status.ok())
    {
      std::cout << "Connection lost, try to reconnect..." << std::endl;
      const bool res = reconnect();
      std::cout << "Reconnect " << (res ? "succeed!" : "failed!") << std::endl;
    }

    return status;
  }

  grpc::Status
  Client::createPartition(mykafka::TopicPartitionRequest& request,
                          mykafka::Error& response)
  {
    grpc::ClientContext context;
    if (client_connection_timeout_ > 0)
      context.set_deadline(std::chrono::system_clock::now() +
                           std::chrono::milliseconds(client_connection_timeout_));

    grpc::Status status = stub_->CreatePartition(&context, request, &response);
    return status;
  }

  grpc::Status
  Client::deletePartition(mykafka::TopicPartitionRequest& request,
                          mykafka::Error& response)
  {    grpc::ClientContext context;
    if (client_connection_timeout_ > 0)
      context.set_deadline(std::chrono::system_clock::now() +
                           std::chrono::milliseconds(client_connection_timeout_));

    grpc::Status status = stub_->DeletePartition(&context, request, &response);
    return status;
  }

  grpc::Status
  Client::deleteTopic(mykafka::TopicPartitionRequest& request,
                      mykafka::Error& response)
  {
    grpc::ClientContext context;
    if (client_connection_timeout_ > 0)
      context.set_deadline(std::chrono::system_clock::now() +
                           std::chrono::milliseconds(client_connection_timeout_));

    grpc::Status status = stub_->DeleteTopic(&context, request, &response);
    return status;
  }


  grpc::Status
  Client::brokerInfo(mykafka::Void& request,
                     mykafka::BrokerInfoResponse& response)
  {
    grpc::ClientContext context;
    if (client_connection_timeout_ > 0)
      context.set_deadline(std::chrono::system_clock::now() +
                           std::chrono::milliseconds(client_connection_timeout_));

    grpc::Status status = stub_->BrokerInfo(&context, request, &response);
    return status;
  }
} // Network

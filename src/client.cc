#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <thread>
#include <chrono>
#include <inttypes.h>

#include "mykafka.grpc.pb.h"

const uint64_t client_connection_timeout = 200; // ms

class BrokerClient
{
public:
  explicit BrokerClient(std::shared_ptr<grpc::Channel> channel, int max_call)
    : max_call_(max_call),
      received_msg_(0),
      channel_(channel),
      stub_(mykafka::Broker::NewStub(channel))
  {
  }

  int64_t sendMessage(const std::string& user)
  {
    mykafka::SendMessageRequest request;
    mykafka::SendMessageResponse response;
    request.set_payload(user);

    grpc::ClientContext context;
    std::chrono::system_clock::time_point deadline =
      std::chrono::system_clock::now() + std::chrono::milliseconds(client_connection_timeout);
    context.set_deadline(deadline);

    grpc::Status status = stub_->SendMessage(&context, request, &response);
    if (status.ok())
      return response.offset();

    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;

    // Try to reconnect
    channel_->GetState(true);

    channel_ = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    stub_ = mykafka::Broker::NewStub(channel_);

    return -1; // RPC failed
  }

private:
  int max_call_;
  int received_msg_;
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<mykafka::Broker::Stub> stub_;
};

int main(int argc, char** argv)
{
  int max_call = -1;
  if (argc >= 2)
    max_call = std::atoi(argv[1]);

  BrokerClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()), max_call);

  for (int i = 0; max_call == -1 || i < max_call; ++i)
  {
    std::string user("world " + std::to_string(i));
    const int64_t offset = client.sendMessage(user);
    std::cout << offset << std::endl;
  }

  return 0;
}

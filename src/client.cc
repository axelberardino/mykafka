#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <thread>
#include <chrono>

#include "mykafka.grpc.pb.h"

class GreeterClient
{
public:
  explicit GreeterClient(std::shared_ptr<grpc::Channel> channel, int max_call)
    : max_call_(max_call),
      received_msg_(0),
      stub_(mykafka::Greeter::NewStub(channel))
  {
  }

  // Assembles the client's payload and sends it to the server.
  void SayHello(const std::string& user)
  {
    // Data we are sending to the server.
    mykafka::HelloRequest request;
    request.set_name(user);

    // Call object to store rpc data
    AsyncClientCall* call = new AsyncClientCall;

    // stub_->AsyncSayHello() performs the RPC call, returning an instance to
    // store in "call". Because we are using the asynchronous API, we need to
    // hold on to the "call" instance in order to get updates on the ongoing RPC.
    call->response_reader = stub_->AsyncSayHello(&call->context, request, &cq_);

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the operation
    // was successful. Tag the request with the memory address of the call object.
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
  }

  // Loop while listening for completed responses.
  // Prints out the response from the server.
  void AsyncCompleteRpc()
  {
    void* got_tag;
    bool ok = false;
    bool stop = false;

    // Block until the next result is available in the completion queue "cq".
    while (!stop && cq_.Next(&got_tag, &ok))
    {
      // The tag in this example is the memory location of the call object
      AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

      // Verify that the request was completed successfully. Note that "ok"
      // corresponds solely to the request for updates introduced by Finish().
      GPR_ASSERT(ok);

      if (call->status.ok())
      {
        ++received_msg_;
        std::cout << "Greeter received: " << call->reply.message() << std::endl;
        if (max_call_ > 0 && received_msg_ >= max_call_)
          stop = true;
      }
      else
        std::cout << "RPC failed" << std::endl;

      // Once we're complete, deallocate the call object.
      delete call;
    }
  }

private:

  // struct for keeping state and data information
  struct AsyncClientCall
  {
    // Container for the data we expect from the server.
    mykafka::HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // Storage for the status of the RPC upon completion.
    grpc::Status status;

    std::unique_ptr<grpc::ClientAsyncResponseReader<mykafka::HelloReply>> response_reader;
  };

  int max_call_;
  int received_msg_;

  // Out of the passed in Channel comes the stub, stored here, our view of the
  // server's exposed services.
  std::unique_ptr<mykafka::Greeter::Stub> stub_;

  // The producer-consumer queue we use to communicate asynchronously with the
  // gRPC runtime.
  grpc::CompletionQueue cq_;
};

int main(int argc, char** argv)
{
  int max_call = -1;
  if (argc >= 2)
    max_call = std::atoi(argv[1]);

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  GreeterClient greeter(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()), max_call);

  // Spawn reader thread that loops indefinitely
  std::thread thread_ = std::thread(&GreeterClient::AsyncCompleteRpc, &greeter);

  for (int i = 0; max_call == -1 || i < max_call; ++i)
  {
    std::string user("world " + std::to_string(i));
    greeter.SayHello(user); // The actual RPC call!
    std::this_thread::sleep_for(std::chrono::microseconds(50));
  }

  std::cout << "Press control-c to quit" << std::endl << std::endl;
  thread_.join();  //blocks forever

  return 0;
}

#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpc++/grpc++.h>
#include <grpc/support/log.h>

#include "mykafka.grpc.pb.h"

#define DEBUG
#undef DEBUG

/*
class GreeterServiceImpl final : public mykafka::Greeter::Service
{
  grpc::Status SayHello(grpc::ServerContext* context, const mykafka::HelloRequest* request,
                        mykafka::HelloReply* reply) override
  {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return grpc::Status::OK;
  }
};

void RunServer()
{
  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl service;

  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main()
{
  RunServer();

  return 0;
}
*/

class ServerImpl final
{
 public:
  ~ServerImpl()
  {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void run()
  {
    std::string server_address("0.0.0.0:50051");

    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    handleRpcs();
  }

 private:
  // Class encompasing the state and logic needed to serve a request.
  class CallData
  {
  public:
    // Take in the "service" instance (in this case representing an asynchronous
    // server) and the completion queue "cq" used for asynchronous communication
    // with the gRPC runtime.
    CallData(mykafka::Greeter::AsyncService* service, grpc::ServerCompletionQueue* cq)
      : client_id_(0), service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
    {
      // Invoke the serving logic right away.
#ifdef DEBUG
      std::cout << "Create " << this << std::endl;
#endif
      proceed();
    }

    void proceed()
    {
      if (status_ == CREATE)
      {
#ifdef DEBUG
        std::cout << "CREATE " << this << std::endl;
#endif
        // Make this instance progress to the PROCESS state.
        status_ = PROCESS;

        // As part of the initial CREATE state, we *request* that the system
        // start processing SayHello requests. In this request, "this" acts are
        // the tag uniquely identifying the request (so that different CallData
        // instances can serve different requests concurrently), in this case
        // the memory address of this CallData instance.
        service_->RequestSayHello(&ctx_, &request_, &responder_, cq_, cq_, this);
      } else if (status_ == PROCESS)
      {
#ifdef DEBUG
        std::cout << "PROCESS " << this << std::endl;
#endif
        // Spawn a new CallData instance to serve new clients while we process
        // the one for this CallData. The instance will deallocate itself as
        // part of its FINISH state.
        new CallData(service_, cq_);

        // The actual processing.
        ++client_id_;
        std::string msg = "Hello ";
        msg += request_.name();
        msg += " (";
        msg += std::to_string(client_id_);
        msg += ")";
        reply_.set_message(msg);

        // And we are done! Let the gRPC runtime know we've finished, using the
        // memory address of this instance as the uniquely identifying tag for
        // the event.
        status_ = FINISH;
        responder_.Finish(reply_, grpc::Status::OK, this);
      }
      else
      {
        GPR_ASSERT(status_ == FINISH);
        // Once in the FINISH state, deallocate ourselves (CallData).
#ifdef DEBUG
        std::cout << "Delete " << this << std::endl;
#endif
        delete this;
      }
    }

  private:
    int client_id_;

    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    mykafka::Greeter::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    grpc::ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    grpc::ServerContext ctx_;

    // What we get from the client.
    mykafka::HelloRequest request_;
    // What we send back to the client.
    mykafka::HelloReply reply_;

    // The means to get back to the client.
    grpc::ServerAsyncResponseWriter<mykafka::HelloReply> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
  };

  // This can be run in multiple threads if needed.
  void handleRpcs()
  {
    // Spawn a new CallData instance to serve new clients.
    new CallData(&service_, cq_.get());
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true)
    {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallData instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->proceed();
    }
  }

  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  mykafka::Greeter::AsyncService service_;
  std::unique_ptr<grpc::Server> server_;
};

int main()
{
  ServerImpl server;
  server.run();

  return 0;
}

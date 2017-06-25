#include "network/Server.hh"

#include "network/SendMessageService.hh"

namespace Network
{
  Server::Server()
  {
  }

  Server::~Server()
  {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  void
  Server::run()
  {
    const std::string server_address("0.0.0.0:50051");

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    handleRpcs();
  }

  void
  Server::handleRpcs()
  {
    // new HelloCallData(&service_, cq_.get());
    // new ByeCallData(&service_, cq_.get());
    new SendMessageService(&service_, cq_.get());
    void* tag = 0;
    bool ok = false;

    while (true)
    {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a Service instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<Service*>(tag)->proceed();
    }
  }
} // Network

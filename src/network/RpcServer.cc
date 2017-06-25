#include "network/RpcServer.hh"
#include "network/Service.hh"

#include <thread>
#include <vector>

namespace Network
{
  RpcServer::RpcServer(std::string address,
                       std::shared_ptr<grpc::Service> service,
                       int32_t thread_number)
    : started_(false),
      thread_number_(thread_number ? thread_number : std::thread::hardware_concurrency()),
      address_(address), service_(service)
  {
  }

  RpcServer::~RpcServer()
  {
    if (started_)
    {
      server_->Shutdown();
      // Always shutdown the completion queue after the server.
      cq_->Shutdown();
    }
  }

  void
  RpcServer::run()
  {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    builder.RegisterService(service_.get());
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    started_ = true;

    std::cout << "Server listening on " << address_
              << " using " << thread_number_ << " threads"
              << std::endl;

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < thread_number_; ++i)
      threads.emplace_back(std::thread([this]() { handleRpcs(); }));
    for (auto& thread : threads)
      thread.join();
  }

  void
  RpcServer::handleRpcs()
  {
    specificHandle();
    void* tag = 0;
    bool ok = false;

    while (true)
    {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a Service instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      if (cq_->Next(&tag, &ok) && ok)
        static_cast<Service*>(tag)->proceed();
    }
  }
} // Network

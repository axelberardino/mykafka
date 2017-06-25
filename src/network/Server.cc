#include "network/Server.hh"
#include "network/SendMessageService.hh"
#include "network/GetMessageService.hh"

#include <thread>
#include <vector>

namespace Network
{
  Server::Server(std::string address, int32_t thread_number)
    : started_(false),
      thread_number_(thread_number ? thread_number : std::thread::hardware_concurrency()),
      address_(address)
  {
  }

  Server::~Server()
  {
    if (started_)
    {
      server_->Shutdown();
      // Always shutdown the completion queue after the server.
      cq_->Shutdown();
    }
  }

  void
  Server::run()
  {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
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
  Server::handleRpcs()
  {
    new SendMessageService(&service_, cq_.get());
    new GetMessageService(&service_, cq_.get());
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

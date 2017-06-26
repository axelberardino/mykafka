#include "broker/Broker.hh"
#include "utils/Utils.hh"

#include <memory>
#include <cassert>

namespace Broker
{
  const std::string CONFIG_FILENAME = "broker.conf";

  Broker::Broker(const std::string& base_path)
    : base_path_(base_path)
  {
  }

  Broker::~Broker()
  {
  }

  mykafka::Error
  Broker::load()
  {
    // Info info{4096, 0, 0, 0, 0, 0, 0,
    //     std::vector<std::string>(), std::vector<std::string>(),
    //     std::make_shared<CommitLog::Partition>(base_path_ + "/test", 0, 0, 0)};

    auto partition = std::make_shared<CommitLog::Partition>(base_path_ + "/default-0", 0, 0, 0);
    auto res = partition->open();
    if (res.code() != mykafka::Error::OK)
      return res;

    topics_["default"][0].partition = partition;

    return Utils::err(mykafka::Error::OK);
  }

  bool
  Broker::loadConf()
  {
    // conf reader class
    return true;
  }

  void
  Broker::getMessage(mykafka::GetMessageRequest& request,
                     mykafka::GetMessageResponse& response)
  {
    auto partition = topics_["default"][0].partition;
    assert(partition);

    std::cout << "Get message from topic " << request.topic()
              << " at offset: " << request.offset() << std::endl;

    std::vector<char> payload;
    auto res = partition->readAt(payload, request.offset());
    auto error = response.error();
    error.set_code(res.code());
    error.set_msg(res.msg());
    response.set_payload(std::string(payload.begin(), payload.end()));
  }

  void
  Broker::sendMessage(mykafka::SendMessageRequest& request,
                      mykafka::SendMessageResponse& response)
  {
    auto partition = topics_["default"][0].partition;
    assert(partition);

    std::cout << "Send to topic " << request.topic() << "-"
              << request.partition() << ": "
              << request.payload() << std::endl;

    std::vector<char> payload(request.payload().begin(), request.payload().end());
    int64_t offset = 0;
    auto res = partition->write(payload, offset);
    auto error = response.error();
    error.set_code(res.code());
    error.set_msg(res.msg());
    response.set_offset(offset);
  }
} // Broker

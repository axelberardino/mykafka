#include "network/Client.hh"
#include "ClientHelpers.hh"

#include <iostream>
#include <inttypes.h>
#include <thread>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
  std::string address;
  std::string topic;
  int32_t partition;
  int64_t offset;
  int64_t nb_offset;

  po::options_description desc("Kafka consumer");
  desc.add_options()
    ("help", "Produce help message")
    ("broker-address",
     po::value<std::string>(&address)->default_value("localhost:9000"), "Set the broker address")
    ("topic", po::value<std::string>(&topic)->default_value("default"), "Set the topic")
    ("offset", po::value<int64_t>(&offset)->default_value(-1), "Set the starting offset (-1 deduce start)")
    ("nb_offset", po::value<int64_t>(&nb_offset)->default_value(0),
     "Set the max number of offset to read (0 = no limit)")
    ("partition", po::value<int32_t>(&partition)->default_value(0), "Set the partition")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 1;
  }

  CHECK_TOPIC;
  CHECK_PARTITION;

  Network::Client client(address);
  if (offset < 0)
  {
    mykafka::GetOffsetsRequest request;
    mykafka::GetOffsetsResponse response;
    request.set_topic(topic);
    request.set_partition(partition);

    auto res = client.getOffsets(request, response);
    CHECK_ERROR("get offsets", response.error().code(), response.error().msg());

    std::cout << "First offset: " << response.first_offset()
              << ", commit_offset: " << response.commit_offset()
              << ", last_offset: " << response.last_offset()
              << std::endl;
    offset = response.first_offset();
  }

  std::cout << "Start to receive from " << address << std::endl;

  bool stop = false;
  while (!stop)
  {
    mykafka::GetMessageRequest request;
    mykafka::GetMessageResponse response;
    request.set_topic(topic);
    request.set_partition(partition);
    request.set_offset(offset);
    auto res = client.getMessage(request, response);
    if (res.ok())
      std::cout << "Payload at offset " << offset << ": " << response.payload() << std::endl;
    else
      std::cout << res.error_code() << ": " << res.error_message() << std::endl;

    switch (response.error().code())
    {
      case mykafka::Error::OK:
        break;
      case mykafka::Error::NO_MESSAGE:
        {
          std::cout << "No message to fetch, waiting 2 sec..." << std::endl;
          std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
        break;
      default:
        {
          std::cout << response.error().code() << ": "
                    << response.error().msg() << std::endl;
          stop = true;
        }
    };
    ++offset;
    if (offset >= nb_offset)
      stop = true;
  }

  return 0;
}

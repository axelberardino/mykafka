#include "network/Client.hh"

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
    ("offset", po::value<int64_t>(&offset)->default_value(0), "Set the starting offset")
    ("nb_offset", po::value<int64_t>(&nb_offset)->default_value(0),
     "Set the max number of offset to read (0 = no limit)")
    ("partition", po::value<int32_t>(&partition)->default_value(0), "Set the partition")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << "Usage: cat file | " << argv[0] << "\n"
              << desc << std::endl;
    return 1;
  }

  Network::Client client(address);
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

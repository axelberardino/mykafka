#include "network/Client.hh"

#include <iostream>
#include <inttypes.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

#define CHECK_TOPIC                                     \
  do {                                                  \
    if (topic.empty())                                  \
    {                                                   \
      std::cout << "Invalid topic!" << std::endl;       \
      return 2;                                         \
    }                                                   \
  } while (0)

#define CHECK_PARTITION                                 \
  do {                                                  \
    if (partition < 0)                                  \
    {                                                   \
      std::cout << "Invalid partition!" << std::endl;   \
      return 3;                                         \
    }                                                   \
  } while (0)

int main(int argc, char** argv)
{
  int32_t partition;
  std::string address;
  std::string topic;
  std::string action;

  po::options_description desc("Kafka control");
  desc.add_options()
    ("help", "Produce help message")
    ("broker-address",
     po::value<std::string>(&address)->default_value("localhost:9000"), "Set the broker address")
    ("topic", po::value<std::string>(&topic)->default_value(""), "Set the topic")
    ("partition", po::value<int32_t>(&partition)->default_value(-1), "Set the partition")
    ("action", po::value<std::string>(&action)->default_value("info"), "Action: create, delete, info, offsets")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 1;
  }

  Network::Client client(address, 0);
  if (action == "create")
  {
    CHECK_TOPIC;
    CHECK_PARTITION;

    mykafka::TopicPartitionRequest request;
    mykafka::Error response;
    request.set_topic(topic);
    request.set_partition(partition);
    auto res = client.createPartition(request, response);
    if (res.ok())
      std::cout << "Partition " << topic << "/"
                << partition << " created!" << std::endl;
    else
      std::cout << "Can't create partition: " << res.error_code()
                << ": " << res.error_message() << std::endl;
  }
  else if (action == "delete")
  {
    CHECK_TOPIC;

    mykafka::TopicPartitionRequest request;
    mykafka::Error response;
    request.set_topic(topic);
    request.set_partition(partition);

    grpc::Status res;
    if (partition < 0)
      res = client.deleteTopic(request, response);
    else
      res = client.deletePartition(request, response);

    if (res.ok())
      std::cout << "Partition " << topic << "/"
                << partition << " created!" << std::endl;
    else
      std::cout << "Can't create partition: " << res.error_code()
                << ": " << res.error_message() << std::endl;
  }
  else if (action == "info")
  {
    mykafka::Void request;
    mykafka::BrokerInfoResponse response;

    auto res = client.brokerInfo(request, response);
    if (res.ok())
      std::cout << response.dump() << std::endl;
    else
      std::cout << "Can't get info on broker: " << res.error_code()
                << ": " << res.error_message() << std::endl;
  }
  else
  {
    std::cout << "Invalid action!" << std::endl;
    return 4;
  }

  return 0;
}

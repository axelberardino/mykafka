#include "network/Client.hh"
#include "ClientHelpers.hh"

#include <iostream>
#include <inttypes.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
  int32_t partition;
  int64_t max_segment_size;
  int64_t max_partition_size;
  int64_t segment_ttl;
  std::string address;
  std::string topic;
  std::string action;

  po::options_description desc("Kafka control");
  desc.add_options()
    ("help", "Produce help message")
    ("broker-address",
     po::value<std::string>(&address)->default_value("localhost:9000"), "Set the broker address")
    ("action", po::value<std::string>(&action)->default_value("info"), "Action: create, delete, info, offsets")
    ("topic", po::value<std::string>(&topic)->default_value(""), "Set the topic")
    ("partition", po::value<int32_t>(&partition)->default_value(-1), "Set the partition")
    ("sement-size", po::value<int64_t>(&max_segment_size)->default_value(4096 * 1024),
     "Set the segment size (4 Mo by default)")
    ("partition-size", po::value<int64_t>(&max_partition_size)->default_value(0),
     "Set the partition max size (0 = no max limit). Older segment will be destroy.")
    ("segment-ttl", po::value<int64_t>(&segment_ttl)->default_value(0),
     "Set the segment ttl in seconds (0 = no ttl). Segment older than ttl will be destroy.")
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
    request.set_max_segment_size(max_segment_size);
    request.set_max_partition_size(max_partition_size);
    request.set_segment_ttl(segment_ttl);
    auto res = client.createPartition(request, response);
    CHECK_ERROR("create partition", response.code(), response.msg());

    std::cout << "Partition " << topic << "/"
              << partition << " created!" << std::endl;
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
    CHECK_ERROR("delete topic/partition", response.code(), response.msg());

    std::cout << "Partition " << topic << "/"
              << partition << " created!" << std::endl;
  }
  else if (action == "offsets")
  {
    CHECK_TOPIC;
    CHECK_PARTITION;

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
  }
  else if (action == "info")
  {
    mykafka::Void request;
    mykafka::BrokerInfoResponse response;

    auto res = client.brokerInfo(request, response);
    CHECK_ERROR("get info", response.error().code(), response.error().msg());

    std::cout << response.dump() << std::endl;
  }
  else
  {
    std::cout << "Invalid action!" << std::endl;
    return 4;
  }

  return 0;
}

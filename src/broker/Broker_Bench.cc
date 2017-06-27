#include "broker/Broker.hh"
#include "utils/Utils.hh"
#include "boost_test_helper.hh"

#include <iostream>
#include <boost/filesystem.hpp>
#include <inttypes.h>
#include <thread>
#include <array>

namespace
{
  void readFrom(Broker::Broker& broker, const std::string& topic,
                int32_t partition, int64_t offset, int64_t nb)
  {
    mykafka::GetMessageRequest request;
    request.set_topic(topic);
    request.set_partition(partition);
    request.set_offset(offset);
    mykafka::GetMessageResponse response;

    for (int i = 0; i < nb; ++i)
      broker.getMessage(request, response);
  }

  void writeFrom(Broker::Broker& broker, const std::string& topic,
                 int32_t partition, const std::string& payload, int64_t nb)
  {
    mykafka::SendMessageRequest request;
    request.set_topic(topic);
    request.set_partition(partition);
    request.set_payload(payload);
    mykafka::SendMessageResponse response;

    for (int i = 0; i < nb; ++i)
      broker.sendMessage(request, response);
  }
} // namespace

int main()
{
  namespace fs = boost::filesystem;

  const std::string tmp_path = "/tmp/mykafka-test/broker";
  const std::string topic = "parallel_delpartition_readwrite";
  const int32_t partition = 0;

  fs::remove_all(tmp_path);
  fs::create_directories(tmp_path);

  Broker::Broker broker(tmp_path);
  for (int i = 0; i < 1000; ++i)
  {
    mykafka::TopicPartitionRequest request;
    request.set_topic(topic);
    request.set_max_segment_size(4096);
    request.set_max_partition_size(0);
    request.set_segment_ttl(0);
    request.set_partition(i);
    broker.createPartition(request);
  }

  writeFrom(broker, topic, partition, "some data", 200);
  readFrom(broker, topic, partition, 0, 200);

  std::vector<std::thread> threads;
  // threads.emplace_back(std::thread([&broker,topic]() {
  //       for (int i = 500; i < 1000; ++i)
  //       {
  //         mykafka::TopicPartitionRequest request;
  //         request.set_topic(topic);
  //         request.set_max_segment_size(4096);
  //         request.set_max_partition_size(0);
  //         request.set_segment_ttl(0);
  //         request.set_partition(i);
  //         broker.deletePartition(request);
  //       }
  //     }));

  for (int i = 0; i < 4; ++i)
  {
    // threads.emplace_back(std::thread([&broker,topic,partition]() {
    //       writeFrom(broker, topic, partition, "some data", 200);
    //     }));
    // threads.emplace_back(std::thread([&broker,topic,partition]() {
    //       writeFrom(broker, topic, partition, "some data", 200);
    //     }));
    threads.emplace_back(std::thread([&broker,topic,partition]() {
          readFrom(broker, topic, partition, 0, 200);
        }));
    threads.emplace_back(std::thread([&broker,topic,partition]() {
          readFrom(broker, topic, partition, 0, 200);
        }));
  }
  for (auto& thread : threads)
    thread.join();

  broker.close();

  return 0;
}

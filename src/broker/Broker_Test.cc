#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Broker/Broker
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "broker/Broker.hh"
#include "utils/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <thread>
#include <array>

namespace
{
  namespace fs = boost::filesystem;

  const std::string tmp_path = "/tmp/mykafka-test/broker";

  struct Setup
  {
    Setup() { fs::remove_all(tmp_path); fs::create_directories(tmp_path); }
    ~Setup() {}
  };

  void createOnePartition(const std::string& topic, int32_t partition)
  {
    Broker::Broker broker(tmp_path);

    mykafka::TopicPartitionRequest request;
    request.set_topic(topic);
    request.set_partition(partition);
    request.set_max_segment_size(4096);
    request.set_max_partition_size(0);
    request.set_segment_ttl(0);

    auto res = broker.createPartition(request);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    BOOST_CHECK_EQUAL(broker.nbTopics(), 1);
    BOOST_CHECK_EQUAL(broker.nbPartitions(), 1);

    res = broker.close();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  }

  void readFrom(Broker::Broker& broker, const std::string& topic,
                int32_t partition, int64_t offset, int64_t nb)
  {
    mykafka::GetMessageRequest request;
    request.set_topic(topic);
    request.set_partition(partition);
    request.set_offset(offset);
    mykafka::GetMessageResponse response;

    for (int i = 0; i < nb; ++i)
    {
      broker.getMessage(request, response);
      auto res = response.error();
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      BOOST_CHECK_EQUAL(response.payload(), "some data");
    }
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
    {
      broker.sendMessage(request, response);
      auto res = response.error();
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    }
  }
} // namespace

BOOST_GLOBAL_FIXTURE(Setup);

BOOST_FIXTURE_TEST_CASE(test_nothing_to_load, Setup)
{
  createOnePartition("create", 1);
}

BOOST_FIXTURE_TEST_CASE(test_create_one_partition, Setup)
{
  Broker::Broker broker(tmp_path);

  mykafka::TopicPartitionRequest request;
  request.set_topic("create");
  request.set_partition(1);
  request.set_max_segment_size(4096);
  request.set_max_partition_size(0);
  request.set_segment_ttl(0);

  auto res = broker.createPartition(request);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  BOOST_CHECK_EQUAL(broker.nbTopics(), 1);
  BOOST_CHECK_EQUAL(broker.nbPartitions(), 1);

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_FIXTURE_TEST_CASE(test_create_many_partition_one_topic, Setup)
{
  Broker::Broker broker(tmp_path);

  mykafka::TopicPartitionRequest request;
  request.set_topic("many");
  request.set_max_segment_size(4096);
  request.set_max_partition_size(0);
  request.set_segment_ttl(0);

  for (int32_t i = 0; i < 10; ++i)
  {
    request.set_partition(i);
    auto res = broker.createPartition(request);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    BOOST_CHECK_EQUAL(broker.nbTopics(), 1);
    BOOST_CHECK_EQUAL(broker.nbPartitions(), i + 1);
  }

  auto res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_FIXTURE_TEST_CASE(test_create_many_partition_many_topic, Setup)
{
  Broker::Broker broker(tmp_path);

  mykafka::TopicPartitionRequest request;
  request.set_max_segment_size(4096);
  request.set_max_partition_size(0);
  request.set_segment_ttl(0);

  for (int32_t i = 0; i < 10; ++i)
  {
    request.set_topic("many_topic_" + std::to_string(i));
    request.set_partition(i);
    auto res = broker.createPartition(request);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    BOOST_CHECK_EQUAL(broker.nbTopics(), i + 1);
    BOOST_CHECK_EQUAL(broker.nbPartitions(), i + 1);
  }

  auto res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_FIXTURE_TEST_CASE(test_delete_partition, Setup)
{
  Broker::Broker broker(tmp_path);

  mykafka::TopicPartitionRequest request;
  request.set_topic("to-delete");
  request.set_partition(1);
  request.set_max_segment_size(4096);
  request.set_max_partition_size(0);
  request.set_segment_ttl(0);

  auto res = broker.createPartition(request);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(broker.nbTopics(), 1);
  BOOST_CHECK_EQUAL(broker.nbPartitions(), 1);

  res = broker.deletePartition(request);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(broker.nbTopics(), 0);
  BOOST_CHECK_EQUAL(broker.nbPartitions(), 0);

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_FIXTURE_TEST_CASE(test_delete_topic, Setup)
{
  Broker::Broker broker(tmp_path);

  mykafka::TopicPartitionRequest request;
  request.set_max_segment_size(4096);
  request.set_max_partition_size(0);
  request.set_segment_ttl(0);

  for (int32_t i = 0; i < 10; ++i)
  {
    request.set_topic("to_delete_topic");
    request.set_partition(i);
    auto res = broker.createPartition(request);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    BOOST_CHECK_EQUAL(broker.nbTopics(), 1);
    BOOST_CHECK_EQUAL(broker.nbPartitions(), i + 1);
  }
  for (int32_t i = 0; i < 10; ++i)
  {
    request.set_topic("do_not_delete_topic");
    request.set_partition(i);
    auto res = broker.createPartition(request);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    BOOST_CHECK_EQUAL(broker.nbTopics(), 2);
    BOOST_CHECK_EQUAL(broker.nbPartitions(), i + 11);
  }
  for (int32_t i = 0; i < 10; ++i)
  {
    // Test for unwanted delete: similar topic name
    request.set_topic("to_delete_topic-0");
    request.set_partition(i);
    auto res = broker.createPartition(request);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    BOOST_CHECK_EQUAL(broker.nbTopics(), 3);
    BOOST_CHECK_EQUAL(broker.nbPartitions(), i + 21);
  }

  request.set_topic("to_delete_topic");
  auto res = broker.deleteTopic(request);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(broker.nbTopics(), 2);
  BOOST_CHECK_EQUAL(broker.nbPartitions(), 20);

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_FIXTURE_TEST_CASE(test_write, Setup)
{
  const std::string topic = "test_readwrite";
  const int32_t partition = 0;
  createOnePartition(topic, partition);

  mykafka::SendMessageRequest request;
  request.set_producer_id(0);
  request.set_group_id("");
  request.set_topic(topic);
  request.set_partition(partition);
  request.set_payload("some data");
  mykafka::SendMessageResponse response;

  Broker::Broker broker(tmp_path);
  auto res = broker.load();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  broker.sendMessage(request, response);
  res = response.error();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(response.offset(), 0);

  broker.sendMessage(request, response);
  res = response.error();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(response.offset(), 1);

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_AUTO_TEST_CASE(test_read)
{
  const std::string topic = "test_readwrite";
  const int32_t partition = 0;

  mykafka::GetMessageRequest request;
  request.set_consumer_id(0);
  request.set_group_id("");
  request.set_topic(topic);
  request.set_partition(partition);
  mykafka::GetMessageResponse response;

  Broker::Broker broker(tmp_path);
  auto res = broker.load();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  request.set_offset(0);
  broker.getMessage(request, response);
  res = response.error();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(response.payload(), "some data");

  request.set_offset(1);
  broker.getMessage(request, response);
  res = response.error();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(response.payload(), "some data");

  request.set_offset(2);
  broker.getMessage(request, response);
  res = response.error();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::NO_MESSAGE, res.msg());
  BOOST_CHECK_EQUAL(response.payload(), "some data");

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_AUTO_TEST_CASE(test_offset)
{
  const std::string topic = "test_readwrite";
  const int32_t partition = 0;

  mykafka::GetOffsetsRequest request;
  request.set_topic(topic);
  request.set_partition(partition);
  mykafka::GetOffsetsResponse response;

  Broker::Broker broker(tmp_path);
  auto res = broker.load();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  broker.getOffsets(request, response);
  res = response.error();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(response.first_offset(), 0);
  BOOST_CHECK_EQUAL(response.commit_offset(), 1);
  BOOST_CHECK_EQUAL(response.last_offset(), 1);

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

// ============================

BOOST_AUTO_TEST_CASE(test_parallel_read_write)
{
  const std::string topic = "parallel_readwrite";
  const int32_t partition = 0;

  createOnePartition(topic, partition);
  Broker::Broker broker(tmp_path);
  auto res = broker.load();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  writeFrom(broker, topic, partition, "some data", 200);
  readFrom(broker, topic, partition, 0, 200);

  std::vector<std::thread> threads;
  for (int i = 0; i < 4; ++i)
  {
    threads.emplace_back(std::thread([&]() {
          writeFrom(broker, topic, partition, "some data", 200);
        }));
    threads.emplace_back(std::thread([&]() {
          readFrom(broker, topic, partition, 0, 200);
        }));
  }
  for (auto& thread : threads)
    thread.join();

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_FIXTURE_TEST_CASE(test_add_topic_while_read_write, Setup)
{
  const std::string topic = "parallel_readwrite";
  const int32_t partition = 0;

  createOnePartition(topic, partition);
  Broker::Broker broker(tmp_path);
  auto res = broker.load();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  writeFrom(broker, topic, partition, "some data", 200);
  readFrom(broker, topic, partition, 0, 200);

  mykafka::TopicPartitionRequest request;
  request.set_topic(topic);
  request.set_max_segment_size(4096);
  request.set_max_partition_size(0);
  request.set_segment_ttl(0);

  std::vector<std::thread> threads;
  threads.emplace_back(std::thread([&]() {
        for (int i = 0; i < 200; ++i)
        {
          request.set_partition(i);
          auto res = broker.createPartition(request);
          BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
        }
      }));

  for (int i = 0; i < 4; ++i)
  {
    threads.emplace_back(std::thread([&]() {
          writeFrom(broker, topic, partition, "some data", 200);
        }));
    threads.emplace_back(std::thread([&]() {
          writeFrom(broker, topic, partition, "some data", 200);
        }));
    threads.emplace_back(std::thread([&]() {
          readFrom(broker, topic, partition, 0, 200);
        }));
  }
  for (auto& thread : threads)
    thread.join();

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_AUTO_TEST_CASE(test_delete_partition_while_read_write)
{
  const std::string topic = "parallel_readwrite";
  const int32_t partition = 0;

  Broker::Broker broker(tmp_path);
  auto res = broker.load();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  writeFrom(broker, topic, partition, "some data", 200);
  readFrom(broker, topic, partition, 0, 200);

  mykafka::TopicPartitionRequest request;
  request.set_topic(topic);
  request.set_max_segment_size(4096);
  request.set_max_partition_size(0);
  request.set_segment_ttl(0);

  std::vector<std::thread> threads;
  threads.emplace_back(std::thread([&]() {
        for (int i = 0; i < 200; ++i)
        {
          request.set_partition(i);
          auto res = broker.deletePartition(request);
          BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
        }
      }));

  for (int i = 0; i < 4; ++i)
  {
    threads.emplace_back(std::thread([&]() {
          writeFrom(broker, topic, partition, "some data", 200);
        }));
    threads.emplace_back(std::thread([&]() {
          writeFrom(broker, topic, partition, "some data", 200);
        }));
    threads.emplace_back(std::thread([&]() {
          readFrom(broker, topic, partition, 0, 200);
        }));
  }
  for (auto& thread : threads)
    thread.join();

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

// delete topic while read/write

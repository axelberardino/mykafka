#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Broker/Broker
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "broker/Broker.hh"
#include "utils/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
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
} // namespace

BOOST_GLOBAL_FIXTURE(Setup);

BOOST_FIXTURE_TEST_CASE(test_nothing_to_load, Setup)
{
  Broker::Broker broker(tmp_path);

  auto res = broker.load();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  BOOST_CHECK_EQUAL(broker.nbTopics(), 0);
  BOOST_CHECK_EQUAL(broker.nbPartitions(), 0);

  res = broker.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
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

  broker.dump(std::cout);
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

  broker.dump(std::cout);
}

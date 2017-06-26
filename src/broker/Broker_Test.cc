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

BOOST_AUTO_TEST_CASE(test_nothing_to_load)
{
  Broker::Broker broker(tmp_path);

  auto res = broker.load();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  BOOST_CHECK_EQUAL(broker.nbTopics(), 0);
  BOOST_CHECK_EQUAL(broker.nbPartitions(), 0);
}

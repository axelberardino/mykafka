#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Utils/ConfigManager
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "utils/ConfigManager.hh"
#include "utils/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <array>

namespace
{
  namespace fs = boost::filesystem;

  const std::string tmp_path = "/tmp/mykafka-test/config";

    struct Setup
  {
    Setup() { fs::remove_all(tmp_path); fs::create_directories(tmp_path); }
    ~Setup() {}
  };
} // namespace

BOOST_GLOBAL_FIXTURE(Setup);

BOOST_AUTO_TEST_CASE(test_create_conf)
{
  Utils::ConfigManager config(tmp_path);

  auto res = config.create({"simple", 0}, 1, 2, 3);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_AUTO_TEST_CASE(test_read_conf)
{
  Utils::ConfigManager config(tmp_path);

  auto res = config.open({"simple", 0});
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  Utils::ConfigManager::RawInfo info;
  res = config.get({"simple", 0}, info);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(info.max_segment_size, 1);
  BOOST_CHECK_EQUAL(info.max_partition_size, 2);
  BOOST_CHECK_EQUAL(info.segment_ttl, 3);
  BOOST_CHECK_EQUAL(info.reader_offset, 0);
  BOOST_CHECK_EQUAL(info.commit_offset, 0);
  config.dump(std::cout);
  res = config.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_FIXTURE_TEST_CASE(test_create_many_then_load, Setup)
{
  {
    Utils::ConfigManager config(tmp_path);
    for (int i = 0; i < 10; ++i)
    {
      auto res = config.create({"many", i}, 1 * i, 2 * i, 3 * i);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    }
    auto res = config.close();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  }

  Utils::ConfigManager config(tmp_path);
  auto res = config.load();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(config.size(), 10);
  config.dump(std::cout);
  res = config.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_FIXTURE_TEST_CASE(test_create_read_update_read, Setup)
{
  Utils::ConfigManager config(tmp_path);
  Utils::ConfigManager::RawInfo info;

  auto res = config.create({"update", 42}, 1, 2, 3);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  res = config.get({"update", 42}, info);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(info.max_segment_size, 1);
  BOOST_CHECK_EQUAL(info.max_partition_size, 2);
  BOOST_CHECK_EQUAL(info.segment_ttl, 3);
  BOOST_CHECK_EQUAL(info.reader_offset, 0);
  BOOST_CHECK_EQUAL(info.commit_offset, 0);

  info.max_segment_size = 123;
  info.max_partition_size = 456;
  info.segment_ttl = 789;
  info.reader_offset = -1;
  info.commit_offset = -1;
  res = config.update({"update", 42}, info);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  res = config.get({"update", 42}, info);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL(info.max_segment_size, 123);
  BOOST_CHECK_EQUAL(info.max_partition_size, 456);
  BOOST_CHECK_EQUAL(info.segment_ttl, 789);
  BOOST_CHECK_EQUAL(info.reader_offset, -1);
  BOOST_CHECK_EQUAL(info.commit_offset, -1);

  res = config.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

BOOST_AUTO_TEST_CASE(test_remove)
{
  Utils::ConfigManager config(tmp_path);

  auto res = config.create({"to_delete", 1337}, 1, 2, 3);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  res = config.open({"to_delete", 1337});
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  res = config.remove({"to_delete", 1337});
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  res = config.open({"to_delete", 1337});
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::FILE_NOT_FOUND, res.msg());

  res = config.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
}

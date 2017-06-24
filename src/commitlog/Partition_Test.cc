#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CommitLog/Partition
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "commitlog/Partition.hh"
#include "commitlog/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <array>

namespace
{
  const int64_t big_partition_size = 1 * 1024 * 1024 * 1024;
  const std::string tmp_path = "/tmp/mykafka-test/partition-test";
  const std::string little_payload = "{my_payload:test, value:0123456789}";

  struct Setup
  {
    Setup() { boost::filesystem::remove_all(tmp_path); }
    ~Setup() {}
  };

  void writePartition(const std::string& suffix, int64_t nb_payload_to_write,
                      int64_t max_segment_size, int64_t max_partition_size)
  {
    CommitLog::Partition partition(tmp_path + suffix, max_segment_size, max_partition_size);
    auto res = partition.open();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    for (int64_t i = 0; i < nb_payload_to_write; ++i)
    {
      int64_t offset = -1;
      res = partition.write(little_payload, offset);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      BOOST_CHECK(offset != -1);
    }
  }
} // namespace

BOOST_GLOBAL_FIXTURE(Setup);

BOOST_AUTO_TEST_CASE(test_partition_one_segment)
{
  writePartition("/test-1seg", 5, (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                 big_partition_size);
}

BOOST_AUTO_TEST_CASE(test_partition_two_segment)
{
  writePartition("/test-2seg", 15, (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                 big_partition_size);
}

BOOST_AUTO_TEST_CASE(test_partition_ten_segment)
{
  writePartition("/test-10seg", 100, (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                 big_partition_size);
}

// BOOST_AUTO_TEST_CASE(test_partition_thousand_segment)
// {
//   writePartition("/test-1000seg", 10000, (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
//                  big_partition_size);
// }

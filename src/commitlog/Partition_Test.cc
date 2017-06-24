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
#include <thread>

namespace
{
  const int64_t big_partition_size = 1 * 1024 * 1024 * 1024;
  const std::string tmp_path = "/tmp/mykafka-test/partition-test";
  const std::string little_payload = "{my_payload:test, value:carrotandj}";
  const std::vector<char> v_payload(little_payload.begin(), little_payload.end());

  struct Setup
  {
    Setup() { boost::filesystem::remove_all(tmp_path); }
    ~Setup() {}
  };

  void writeFrom(CommitLog::Partition& partition, int64_t nb_payload, bool check_offset = true)
  {
    const int64_t base_offset = partition.newestOffset();
    for (int64_t i = 0; i < nb_payload; ++i)
    {
      int64_t offset = -1;
      auto res = partition.write(v_payload, offset);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      BOOST_CHECK(offset != -1);
      if (check_offset)
        BOOST_CHECK_EQUAL(base_offset + i, offset);
    }
  }

  void readFrom(CommitLog::Partition& partition, int64_t nb_payload)
  {
    const int64_t base_offset = partition.oldestOffset();
    std::vector<char> payload;
    for (int64_t i = 0; i < nb_payload; ++i)
    {
      auto res = partition.readAt(payload, base_offset + i);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      const std::string str_payload(payload.begin(), payload.end());
      BOOST_CHECK_EQUAL(little_payload, str_payload);
    }
  }

  void writeAndReadFrom(CommitLog::Partition& partition, int64_t nb_payload, bool read)
  {
    writeFrom(partition, nb_payload);
    if (read)
      readFrom(partition, nb_payload);
  }

  void writeAndReadPartition(const std::string& suffix, int64_t nb_payload,
                             int64_t max_segment_size, int64_t max_partition_size,
                             bool read, int64_t ttl = 0)
  {
    CommitLog::Partition partition(tmp_path + suffix, max_segment_size,
                                   max_partition_size, ttl);
    auto res = partition.open();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    writeAndReadFrom(partition, nb_payload, read);
  }
} // namespace

BOOST_GLOBAL_FIXTURE(Setup);

BOOST_AUTO_TEST_CASE(test_partition_one_segment)
{
  writeAndReadPartition("/test-1seg", 5,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, false);
}

BOOST_AUTO_TEST_CASE(test_partition_1_segment_10kmsg)
{
  writeAndReadPartition("/test-1seg10k", 10000,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10000,
                        big_partition_size, false);
}

BOOST_AUTO_TEST_CASE(test_partition_two_segment)
{
  writeAndReadPartition("/test-2seg", 15,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, false);
}

BOOST_AUTO_TEST_CASE(test_partition_ten_segment)
{
  writeAndReadPartition("/test-10seg", 100,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, false);
}

// BOOST_AUTO_TEST_CASE(test_partition_thousand_segment)
// {
//   writeAndReadPartition("/test-1000seg", 10000,
//                         (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
//                         big_partition_size, false);
// }

BOOST_AUTO_TEST_CASE(test_partition_one_segment_reopen)
{
  writeAndReadPartition("/test-1reopenseg", 2,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, false);
  writeAndReadPartition("/test-1reopenseg", 2,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, false);
}

BOOST_AUTO_TEST_CASE(test_partition_new_segment_reopen)
{
  writeAndReadPartition("/test-newseg", 20,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, false);
  writeAndReadPartition("/test-newseg", 1,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, false);
  writeAndReadPartition("/test-newseg", 15,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, false);
  writeAndReadPartition("/test-newseg", 30,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, false);
}

// ============================

BOOST_AUTO_TEST_CASE(test_partition_one_segment_with_reread)
{
  writeAndReadPartition("/test-1seg", 5,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, true);
}

BOOST_AUTO_TEST_CASE(test_partition_1_segment_10kmsg_with_reread)
{
  writeAndReadPartition("/test-1seg10k", 10000,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10000,
                        big_partition_size, true);
}

BOOST_AUTO_TEST_CASE(test_partition_two_segment_with_reread)
{
  writeAndReadPartition("/test-2seg", 15,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, true);
}

BOOST_AUTO_TEST_CASE(test_partition_ten_segment_with_reread)
{
  writeAndReadPartition("/test-10seg", 100,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, true);
}

// BOOST_AUTO_TEST_CASE(test_partition_thousand_segment_with_reread)
// {
//   writeAndReadPartition("/test-1000seg", 10000,
//                         (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
//                         big_partition_size, true);
// }

BOOST_AUTO_TEST_CASE(test_partition_one_segment_reopen_with_reread)
{
  writeAndReadPartition("/test-1reopenseg", 2,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, true);
  writeAndReadPartition("/test-1reopenseg", 2,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, true);
}

BOOST_AUTO_TEST_CASE(test_partition_new_segment_reopen_with_reread)
{
  writeAndReadPartition("/test-newseg", 20,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, true);
  writeAndReadPartition("/test-newseg", 1,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, true);
  writeAndReadPartition("/test-newseg", 15,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, true);
  writeAndReadPartition("/test-newseg", 30,
                        (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                        big_partition_size, true);
}

BOOST_AUTO_TEST_CASE(test_partition_multithread)
{
  CommitLog::Partition partition(tmp_path + "/test-multithread",
                                 (little_payload.size() + CommitLog::Segment::HEADER_SIZE) * 10,
                                 big_partition_size, 0);
  auto res = partition.open();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  std::vector<std::thread> threads;
  writeFrom(partition, 200);
  for (int i = 0; i < 4; ++i)
  {
    threads.emplace_back(std::thread([&partition]() {
          writeFrom(partition, 200, false);
        }));
    threads.emplace_back(std::thread([&partition]() {
          readFrom(partition, 200);
        }));
  }
  for (auto& thread : threads)
    thread.join();
}

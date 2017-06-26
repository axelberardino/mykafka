#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CommitLog/Partition
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "commitlog/Partition.hh"
#include "utils/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <array>
#include <thread>

namespace
{
  namespace fs = boost::filesystem;

  const int64_t big_partition_size = 0;
  const std::string tmp_path = "/tmp/mykafka-test/partition-test";
  const std::string little_payload = "{my_payload:test, value:carrotandj}";
  const std::vector<char> v_payload(little_payload.begin(), little_payload.end());
  const int64_t max_segment_size = little_payload.size() + CommitLog::Segment::HEADER_SIZE;

  struct Setup
  {
    Setup() { fs::remove_all(tmp_path); }
    ~Setup() {}
  };

  int64_t countFiles(const std::string& dir)
  {
    return std::count_if(fs::directory_iterator(dir),
                         fs::directory_iterator(),
                         static_cast<bool(*)(const fs::path&)>(fs::is_regular_file));
  }

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

  void writeAndReadPartition(const std::string& path, int64_t nb_payload,
                             int64_t max_segment_size, int64_t max_partition_size,
                             bool read, int64_t ttl = 0)
  {
    CommitLog::Partition partition(path, max_segment_size,
                                   max_partition_size, ttl);
    auto res = partition.open();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    writeAndReadFrom(partition, nb_payload, read);
  }
} // namespace

BOOST_GLOBAL_FIXTURE(Setup);

BOOST_AUTO_TEST_CASE(test_partition_one_segment)
{
  const std::string dir = tmp_path + "/test-1seg";
  writeAndReadPartition(dir, 5,
                        max_segment_size * 10,
                        big_partition_size, false);
  BOOST_CHECK_EQUAL(countFiles(dir), 1 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_1_segment_10kmsg)
{
  const std::string dir = tmp_path + "/test-1seg10k";
  writeAndReadPartition(dir, 10000,
                        max_segment_size * 10000,
                        big_partition_size, false);
  BOOST_CHECK_EQUAL(countFiles(dir), 1 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_two_segment)
{
  const std::string dir = tmp_path + "/test-2seg";
  writeAndReadPartition(dir, 15,
                        max_segment_size * 10,
                        big_partition_size, false);
  BOOST_CHECK_EQUAL(countFiles(dir), 2 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_ten_segment)
{
  const std::string dir = tmp_path + "/test-10seg";
  writeAndReadPartition(dir, 100,
                        max_segment_size * 10,
                        big_partition_size, false);
  BOOST_CHECK_EQUAL(countFiles(dir), 10 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_hundred_segment)
{
  const std::string dir = tmp_path + "/test-100seg";
  writeAndReadPartition(dir, 1100,
                        max_segment_size * 10,
                        big_partition_size, false);
  BOOST_CHECK_EQUAL(countFiles(dir), 100 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_one_segment_reopen)
{
  const std::string dir = tmp_path + "/test-1reopenseg";
  writeAndReadPartition(dir, 2,
                        max_segment_size * 10,
                        big_partition_size, false);
  writeAndReadPartition(dir, 2,
                        max_segment_size * 10,
                        big_partition_size, false);
  BOOST_CHECK_EQUAL(countFiles(dir), 1 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_new_segment_reopen)
{
  const std::string dir = tmp_path + "/test-newseg";
  writeAndReadPartition(dir, 20,
                        max_segment_size * 10,
                        big_partition_size, false);
  writeAndReadPartition(dir, 1,
                        max_segment_size * 10,
                        big_partition_size, false);
  writeAndReadPartition(dir, 15,
                        max_segment_size * 10,
                        big_partition_size, false);
  writeAndReadPartition(dir, 30,
                        max_segment_size * 10,
                        big_partition_size, false);
  BOOST_CHECK_EQUAL(countFiles(dir), 6 * 2);
}

// ============================

BOOST_AUTO_TEST_CASE(test_partition_one_segment_with_reread)
{
  const std::string dir = tmp_path + "/test-r1seg";
  writeAndReadPartition(dir, 5,
                        max_segment_size * 10,
                        big_partition_size, true);
  BOOST_CHECK_EQUAL(countFiles(dir), 1 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_1_segment_10kmsg_with_reread)
{
  const std::string dir = tmp_path + "/test-r1seg10k";
  writeAndReadPartition(dir, 10000,
                        max_segment_size * 10000,
                        big_partition_size, true);
  BOOST_CHECK_EQUAL(countFiles(dir), 1 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_two_segment_with_reread)
{
  const std::string dir = tmp_path + "/test-r2seg";
  writeAndReadPartition(dir, 15,
                        max_segment_size * 10,
                        big_partition_size, true);
  BOOST_CHECK_EQUAL(countFiles(dir), 2 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_ten_segment_with_reread)
{
  const std::string dir = tmp_path + "/test-r10seg";
  writeAndReadPartition(dir, 100,
                        max_segment_size * 10,
                        big_partition_size, true);
  BOOST_CHECK_EQUAL(countFiles(dir), 10 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_hundred_segment_with_reread)
{
  const std::string dir = tmp_path + "/test-r100seg";
  writeAndReadPartition(dir, 1100,
                        max_segment_size * 10,
                        big_partition_size, true);
  BOOST_CHECK_EQUAL(countFiles(dir), 100 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_one_segment_reopen_with_reread)
{
  const std::string dir = tmp_path + "/test-r1reopenseg";
  writeAndReadPartition(dir, 2,
                        max_segment_size * 10,
                        big_partition_size, true);
  writeAndReadPartition(dir, 2,
                        max_segment_size * 10,
                        big_partition_size, true);
  BOOST_CHECK_EQUAL(countFiles(dir), 1 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_new_segment_reopen_with_reread)
{
  const std::string dir = tmp_path + "/test-rnewseg";
  writeAndReadPartition(dir, 20,
                        max_segment_size * 10,
                        big_partition_size, true);
  writeAndReadPartition(dir, 1,
                        max_segment_size * 10,
                        big_partition_size, true);
  writeAndReadPartition(dir, 15,
                        max_segment_size * 10,
                        big_partition_size, true);
  writeAndReadPartition(dir, 30,
                        max_segment_size * 10,
                        big_partition_size, true);
  BOOST_CHECK_EQUAL(countFiles(dir), 6 * 2);
}

// ============================

BOOST_AUTO_TEST_CASE(test_partition_with_max_size)
{
  const std::string dir = tmp_path + "/test-maxsize";
  writeAndReadPartition(dir, 100,
                        max_segment_size * 10,
                        max_segment_size * 10 * 2, false);

  BOOST_CHECK_EQUAL(countFiles(dir), 2 * 2);
}

BOOST_AUTO_TEST_CASE(test_partition_with_ttl)
{
  const std::string dir = tmp_path + "/test-ttl";
  writeAndReadPartition(dir, 50,
                        max_segment_size * 10,
                        big_partition_size, false, 0);

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  writeAndReadPartition(dir, 50,
                        max_segment_size * 10,
                        big_partition_size, false, 1 /* ttl = 1 sec */);

  BOOST_CHECK_EQUAL(countFiles(dir), 5 * 2);
}

// ============================

BOOST_AUTO_TEST_CASE(test_partition_multithread)
{
  const std::string dir = tmp_path + "/test-multithread";
  CommitLog::Partition partition(dir, max_segment_size * 10, big_partition_size, 0);
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

  BOOST_CHECK_EQUAL(countFiles(dir), 91 * 2);
}

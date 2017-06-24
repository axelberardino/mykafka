#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CommitLog/Bench
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "commitlog/Partition.hh"
#include "commitlog/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <array>
#include <vector>
#include <thread>
#include <fstream>
#include <chrono>

namespace
{
  namespace fs = boost::filesystem;

  const int64_t big_partition_size = 0;
  const std::string dico_filename = "/usr/share/dict/words";
  const std::string tmp_path = "/tmp/mykafka-test";
  const std::string partition_path = tmp_path + "/commitlog-bench";
  std::vector<std::vector<char> > dico;

  struct Setup
  {
    Setup()
    {
      fs::remove_all(tmp_path);
      std::ifstream file(dico_filename);
      if (!file)
        return;

      const int64_t nb_line = 100000;
      int64_t i = 0;
      std::string line;
      dico.resize(nb_line);
      while (std::getline(file, line) && i < nb_line)
      {
        dico[i] = std::vector<char>(line.begin(), line.end());
        ++i;
      }
      dico.resize(i);
    }
    ~Setup() {}
  };

  struct PrepareTest
  {
    PrepareTest()
    {
      BOOST_REQUIRE_MESSAGE(!dico.empty(), "Dictionnary is empty, check " +
                            dico_filename + ", which is distributed in"
                            " package \"wbritish\"");
    }
    ~PrepareTest() {}
  };

  int64_t countFiles(const std::string& dir)
  {
    return std::count_if(fs::directory_iterator(dir),
                         fs::directory_iterator(),
                         static_cast<bool(*)(const fs::path&)>(fs::is_regular_file));
  }

  void rangeWrite(CommitLog::Partition& partition, int64_t from, int64_t to)
  {
    for (int64_t i = from; i <= to; ++i)
    {
      int64_t offset = -1;
      auto res = partition.write(dico[i], offset);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      BOOST_CHECK(offset != -1);
    }
  }

  void rangeRead(CommitLog::Partition& partition, int64_t from, int64_t to)
  {
    const int64_t base_offset = partition.oldestOffset();
    std::vector<char> payload;
    for (int64_t i = from; i <= to; ++i)
    {
      auto res = partition.readAt(payload, base_offset + i);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    }
  }

  void testWritePartition(const std::string& suffix, int64_t segment_size)
  {
    const std::string dir = partition_path + suffix;
    CommitLog::Partition partition(dir, segment_size, 0, 0);
    auto res = partition.open();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    auto start = std::chrono::system_clock::now();
    rangeWrite(partition, 0, dico.size() - 1);
    auto end = std::chrono::system_clock::now();
    const int64_t elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << dico.size() << " write in " << elapsed_ms << "ms"
              << " (" << (dico.size() * 1000 / elapsed_ms) << " msg/s)"
              << std::endl;

    // partition.close(); in partition destructor,
    // cost a lot due to syncfs + msync on all index :(.
    // Hence, this test is faster to execute than to close.
    // @see Index.cc Index::close() and Index::sync()

    BOOST_CHECK(countFiles(dir) > 0);
  }

  void testReadPartition(const std::string& suffix, int64_t segment_size)
  {
    const std::string dir = partition_path + suffix;
    CommitLog::Partition partition(dir, segment_size, 0, 0);
    auto res = partition.open();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    auto start = std::chrono::system_clock::now();
    rangeRead(partition, 0, dico.size() - 1);
    auto end = std::chrono::system_clock::now();
    const int64_t elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << dico.size() << " read in " << elapsed_ms << "ms"
              << " (" << (dico.size() * 1000 / elapsed_ms) << " msg/s)"
              << std::endl;
    BOOST_CHECK(countFiles(dir) > 0);
  }

  void testManyWriterAndManyReader(const std::string& suffix, int64_t segment_size)
  {
    const std::string dir = partition_path + suffix;
    CommitLog::Partition partition(dir, segment_size, 0, 0);
    auto res = partition.open();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());


    auto start = std::chrono::system_clock::now();

    std::vector<std::thread> threads;
    rangeWrite(partition, 0, dico.size() - 1);
    for (int i = 0; i < 4; ++i)
    {
      threads.emplace_back(std::thread([&partition]() {
            rangeWrite(partition, 0, dico.size() - 1);
          }));
      threads.emplace_back(std::thread([&partition]() {
            rangeRead(partition, 0, dico.size() - 1);
          }));
    }
    for (auto& thread : threads)
      thread.join();

    auto end = std::chrono::system_clock::now();
    const int64_t elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << (5 * dico.size()) << " write in " << elapsed_ms << "ms"
              << " (" << (5 * dico.size() * 1000 / elapsed_ms) << " msg/s)"
              << std::endl
              << (4 * dico.size()) << " read in " << elapsed_ms << "ms"
              << " (" << (4 * dico.size() * 1000 / elapsed_ms) << " msg/s)"
              << std::endl
              << (9 * dico.size()) << " operation in " << elapsed_ms << "ms"
              << " (" << (9 * dico.size() * 1000 / elapsed_ms) << " msg/s)"
              << std::endl;
    BOOST_CHECK(countFiles(dir) > 0);
  }
} // namespace

BOOST_GLOBAL_FIXTURE(Setup);

BOOST_FIXTURE_TEST_CASE(bench_commitlog_write_single_thread_1K, PrepareTest)
{
  testWritePartition("/simple1k", 1024); // 1 Ko
}

BOOST_FIXTURE_TEST_CASE(bench_commitlog_read_single_thread_1K, PrepareTest)
{
  testReadPartition("/simple1k", 1024); // 1 Ko
}

BOOST_FIXTURE_TEST_CASE(bench_commitlog_write_single_thread_4K, PrepareTest)
{
  testWritePartition("/simple4k", 4096); // 4 Ko
}

BOOST_FIXTURE_TEST_CASE(bench_commitlog_read_single_thread_4K, PrepareTest)
{
  testReadPartition("/simple4k", 4096); // 4 Ko
}

BOOST_FIXTURE_TEST_CASE(bench_commitlog_write_single_thread_4M, PrepareTest)
{
  testWritePartition("/simple4m", 4096 * 1024); // 4 Mo
}

BOOST_FIXTURE_TEST_CASE(bench_commitlog_read_single_thread_4M, PrepareTest)
{
  testReadPartition("/simple4m", 4096 * 1024); // 4 Mo
}

// ============================

BOOST_FIXTURE_TEST_CASE(bench_commitlog_readwrite_multithread_1K, PrepareTest)
{
  testManyWriterAndManyReader("/multithread4k", 1024); // 1 Ko
}

BOOST_FIXTURE_TEST_CASE(bench_commitlog_readwrite_multithread_4K, PrepareTest)
{
  testManyWriterAndManyReader("/multithread4k", 4096); // 4 Ko
}

BOOST_FIXTURE_TEST_CASE(bench_commitlog_readwrite_multithread_4M, PrepareTest)
{
  testManyWriterAndManyReader("/multithread4m", 4096 * 1024); // 4 Mo
}

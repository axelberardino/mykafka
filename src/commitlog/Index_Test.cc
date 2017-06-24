#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CommitLog/Index
#include <boost/test/included/unit_test.hpp>

#include "commitlog/Index.hh"
#include "commitlog/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <vector>

namespace
{
  struct Entry
  {
    int64_t offset;
    int64_t position;
  };

  void testIndex(int64_t base_offset)
  {
    const std::string tmp_file = "/tmp/mykafka-test/test" + std::to_string(base_offset) + ".index";
    const int64_t total_entries = 10;
    const int64_t size = total_entries * CommitLog::Index::ENTRY_WIDTH;
    CommitLog::Index index(tmp_file, base_offset, size);

    if (fileExists(tmp_file))
    {
      auto res = index.deleteIndex();
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    }

    auto res = index.open();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    struct stat buf;
    fstat(index.fd(), &buf);
    std::cout << size << " " << CommitLog::Utils::roundDownToMultiple(size,
                                                                      CommitLog::Index::ENTRY_WIDTH)
              << " " << buf.st_size << std::endl;
    BOOST_CHECK_EQUAL(CommitLog::Utils::roundDownToMultiple(size,
                                                            CommitLog::Index::ENTRY_WIDTH),
                      buf.st_size);

    std::vector<Entry> entries;
    for (int i = 0; i < total_entries; ++i)
      entries.push_back({base_offset + i, i * 123456789});

    for (auto& entry : entries)
    {
      res = index.write(entry.offset, entry.position);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    }
    res = index.sync();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    int i = 0;
    Entry got_entry{0, 0};
    for (auto& entry : entries)
    {
      res = index.read(got_entry.offset, got_entry.position, i * CommitLog::Index::ENTRY_WIDTH);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      BOOST_CHECK_EQUAL(entry.offset, got_entry.offset);
      BOOST_CHECK_EQUAL(entry.position, got_entry.position);
      ++i;
    }
    // Check for overflow detection
    res = index.read(got_entry.offset, got_entry.position,
                     base_offset + i * CommitLog::Index::ENTRY_WIDTH);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::INDEX_ERROR, res.msg());

    res = index.sanityCheck();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    //dirty data
    ++index.position_;
    res = index.sanityCheck();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::INDEX_ERROR, res.msg());
    --index.position_;

    fstat(index.fd(), &buf);
    BOOST_CHECK_EQUAL(total_entries * CommitLog::Index::ENTRY_WIDTH, buf.st_size);

    res = index.close();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    BOOST_CHECK_EQUAL_MSG(index.fd(), -1, "After a close, fd should be at -1");
  }
} // namespace

BOOST_AUTO_TEST_CASE(test_index_offset_0)
{
  testIndex(0);
}

BOOST_AUTO_TEST_CASE(test_index_offset_1)
{
  testIndex(1);
}

BOOST_AUTO_TEST_CASE(test_index_offset_1024)
{
  testIndex(1024);
}

BOOST_AUTO_TEST_CASE(test_index_big_offset)
{
  testIndex(420077);
}

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CommitLog/Index
#include <boost/test/included/unit_test.hpp>

#include "commitlog/Index.hh"
#include "commitlog/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <vector>

struct Entry
{
  int64_t offset;
  int64_t position;
};

BOOST_AUTO_TEST_CASE(test_index)
{
  const std::string tmp_file = "/tmp/test.index";
  const int64_t total_entries = 10;
  const int64_t size = total_entries * CommitLog::Index::ENTRY_WIDTH;
  CommitLog::Index index(tmp_file, size, 0);
  auto res = index.deleteIndex();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  res = index.create();
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
    entries.push_back({i, i * 100});

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
  res = index.read(got_entry.offset, got_entry.position, i * CommitLog::Index::ENTRY_WIDTH);
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::INDEX_ERROR, res.msg());

  res = index.sanityCheck();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  //dirty data
  ++index.position_;
  res = index.sanityCheck();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::INDEX_ERROR, res.msg());
  --index.position_;

  res = index.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  fstat(index.fd(), &buf);
  BOOST_CHECK_EQUAL(total_entries * CommitLog::Index::ENTRY_WIDTH, buf.st_size);
}

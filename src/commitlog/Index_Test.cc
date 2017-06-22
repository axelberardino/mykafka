#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CommitLog/Index
#include <boost/test/included/unit_test.hpp>

#include "commitlog/Index.hh"
#include "commitlog/Utils.hh"
#include <inttypes.h>

BOOST_AUTO_TEST_CASE(test_index)
{
  const std::string tmp_file = "/tmp/test.index";
  const int64_t totalEntries = (rand() % 10) + 10;
  const int64_t bytes = totalEntries * CommitLog::Index::ENTRY_WIDTH + 1;
  CommitLog::Index index;
  auto res = index.create(tmp_file, bytes, 0);
  std::cout << "Index create: " << res.msg() << std::endl;
  BOOST_CHECK_EQUAL(res.code(), mykafka::Error::OK);

  struct stat buf;
  BOOST_CHECK(fstat(index.fd(), &buf) >= 0);
  BOOST_CHECK_EQUAL(CommitLog::Utils::roundDownToMultiple(bytes,
                                                          CommitLog::Index::ENTRY_WIDTH),
                    buf.st_size);
}

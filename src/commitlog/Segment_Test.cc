#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CommitLog/Segment
#include <boost/test/included/unit_test.hpp>

#include "commitlog/Segment.hh"
#include "commitlog/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <array>

namespace
{
  int64_t payloadsSize(const std::array<std::string, 10>& payloads)
  {
    int64_t res = 0;
    for (auto& s : payloads)
      res += s.size() + CommitLog::Segment::HEADER_SIZE;
    return res;
  }
} // namespace

BOOST_AUTO_TEST_CASE(test_segment)
{
  const std::string tmp_path = "/tmp/";
  const std::array<std::string, 10> payloads =
    {
      "{my_payload:apple, value:1}",
      "{my_payload:orange, value:42}",
      "{my_payload:banana, value:5}",
      "{my_payload:raspberries, value:875}",
      "{my_payload:pear, value:381}",
      "{my_payload:pineapple, value:0}",
      "{my_payload:strawberry, value:28756}",
      "{my_payload:cherry, value:4632}",
      "{my_payload:coconut, value:123456789}",
      "{my_payload:mango, value:00}"
    };
  const int64_t size = payloadsSize(payloads);
  CommitLog::Segment segment(tmp_path, 0, size);
  segment.deleteSegment();

  auto res = segment.create();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

  for (auto& payload : payloads)
  {
    res = segment.write(payload);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  }

  struct stat buf;
  fstat(segment.segmentFd(), &buf);
  BOOST_CHECK_EQUAL(size, buf.st_size);

  int offset = 0;
  std::string got_payload;
  for (auto& payload : payloads)
  {
    res = segment.readAt(got_payload, offset);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    BOOST_CHECK_EQUAL(payload, got_payload);
    ++offset;
  }

  res = segment.close();
  BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
  BOOST_CHECK_EQUAL_MSG(segment.segmentFd(), -1,
                        "After a close, segment fd should be at -1");
  BOOST_CHECK_EQUAL_MSG(segment.indexFd(), -1,
                        "After a close, index fd should be at -1");
}

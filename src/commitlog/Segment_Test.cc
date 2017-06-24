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
  template <typename T>
  int64_t payloadsSize(const T& payloads)
  {
    int64_t res = 0;
    for (auto& s : payloads)
      res += s.size() + CommitLog::Segment::HEADER_SIZE;
    return res;
  }

  const std::string tmp_path = "/tmp/mykafka-test";
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

  std::string vecToString(const std::vector<char>& tab)
  {
    std::string res(tab.size(), 0);
    for (uint32_t i = 0; i < tab.size(); ++i)
      res[i] = tab[i];
    res[tab.size()] = 0;

    return res;
  }

  void testSegment(int64_t base_offset)
  {
    const int64_t size = payloadsSize(payloads);
    CommitLog::Segment segment(tmp_path, base_offset, size);
    segment.deleteSegment();

    auto res = segment.open();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    for (auto& payload : payloads)
    {
      res = segment.write(payload);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    }

    struct stat buf;
    fstat(segment.segmentFd(), &buf);
    BOOST_CHECK_EQUAL(size, buf.st_size);

    int64_t rel_offset = -1;
    int64_t rel_position = -1;
    for (int64_t i = 0; i < static_cast<int64_t>(payloads.size()); ++i)
    {
      res = segment.findEntry(rel_offset, rel_position, i);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      BOOST_CHECK_EQUAL(rel_offset, i);
    }
    res = segment.findEntry(rel_offset, rel_position, payloads.size());
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    BOOST_CHECK_EQUAL(rel_offset, -1);

    int offset = 0;
    std::vector<char> raw_got_payload;
    for (auto& payload : payloads)
    {
      res = segment.readAt(raw_got_payload, offset);
      const std::string got_payload = vecToString(raw_got_payload);
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      BOOST_CHECK_EQUAL(payload, got_payload);
      ++offset;
    }

    res = segment.dump(std::cout);
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    res = segment.close();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
    BOOST_CHECK_EQUAL_MSG(segment.segmentFd(), -1,
                          "After a close, segment fd should be at -1");
    BOOST_CHECK_EQUAL_MSG(segment.indexFd(), -1,
                          "After a close, index fd should be at -1");
  }
} // namespace


BOOST_AUTO_TEST_CASE(test_segment_offset_0)
{
  testSegment(0);
}

BOOST_AUTO_TEST_CASE(test_segment_offset_1)
{
  testSegment(1);
}

BOOST_AUTO_TEST_CASE(test_segment_offset_1024)
{
  testSegment(1024);
}

BOOST_AUTO_TEST_CASE(test_segment_big_offset)
{
  testSegment(520053);
}

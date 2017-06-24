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
      "{my_payload:apple, value:a}",
      "{my_payload:orange, value:ab}",
      "{my_payload:banana, value:a}",
      "{my_payload:raspberries, value:abc}",
      "{my_payload:pear, value:cde}",
      "{my_payload:pineapple, value:a}",
      "{my_payload:strawberry, value:ertyu}",
      "{my_payload:cherry, value:glgl}",
      "{my_payload:coconut, value:lvnsqhgdi}",
      "{my_payload:mango, value:xx}"
    };
  const int64_t size = payloadsSize(payloads);

  void testSegment(int64_t base_offset, bool reopen = false)
  {
    const int64_t size = payloadsSize(payloads);
    CommitLog::Segment segment(tmp_path, base_offset, size);
    segment.deleteSegment();

    auto res = segment.open();
    BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());

    for (auto& payload : payloads)
    {
      int64_t written_offset = 0;
      const std::vector<char> v_payload(payload.begin(), payload.end());
      res = segment.write(v_payload, written_offset);
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

    if (reopen)
    {
      const int64_t previous_base_offset = segment.baseOffset();
      const int64_t previous_next_offset = segment.nextOffset();
      res = segment.close();
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      res = segment.open();
      BOOST_CHECK_EQUAL_MSG(res.code(), mykafka::Error::OK, res.msg());
      BOOST_CHECK_EQUAL(previous_base_offset, segment.baseOffset());
      BOOST_CHECK_EQUAL(previous_next_offset, segment.nextOffset());
    }

    int offset = 0;
    std::vector<char> raw_got_payload;
    for (auto& payload : payloads)
    {
      res = segment.readAt(raw_got_payload, offset);
      const std::string got_payload(raw_got_payload.begin(), raw_got_payload.end());
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

BOOST_AUTO_TEST_CASE(test_segment_offset_0_reopen)
{
  testSegment(0, true);
}

BOOST_AUTO_TEST_CASE(test_segment_offset_1_reopen)
{
  testSegment(1, true);
}

BOOST_AUTO_TEST_CASE(test_segment_offset_1024_reopen)
{
  testSegment(1024, true);
}

BOOST_AUTO_TEST_CASE(test_segment_big_offset_reopen)
{
  testSegment(520053, true);
}

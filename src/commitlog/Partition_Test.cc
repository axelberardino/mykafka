#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CommitLog/Partition
#include <boost/test/included/unit_test.hpp>

#include "commitlog/Partition.hh"
#include "commitlog/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <array>

namespace
{
  const std::string tmp_path = "/tmp/mykafka-test/test-partition";
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

  void testPartion()
  {

  }
} // namespace


BOOST_AUTO_TEST_CASE(test_partition)
{
  testPartion();
}

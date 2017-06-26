#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Utils/ConfigManager
#include <boost/test/included/unit_test.hpp>

#include "commitlog/Segment.hh"
#include "utils/Utils.hh"
#include "boost_test_helper.hh"

#include <inttypes.h>
#include <array>

namespace
{
  const std::string tmp_path = "/tmp/mykafka-test/config";

  void test()
  {
  }
} // namespace


BOOST_AUTO_TEST_CASE(test_config)
{
  test();
}

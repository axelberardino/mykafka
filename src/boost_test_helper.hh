#ifndef BOOST_TEST_HELPER_HH_
# define BOOST_TEST_HELPER_HH_

# include <unistd.h>

#define BOOST_CHECK_EQUAL_MSG(Got, Expected, Msg) \
  do {                                            \
  BOOST_CHECK_EQUAL(Got, Expected);               \
  if (Got != Expected)                            \
    std::cout << "  => " << Msg << std::endl;     \
  } while (0)

inline bool fileExists(const std::string& name)
{
  return access(name.c_str(), F_OK) != -1;
}

#endif /* !BOOST_TEST_HELPER_HH_ */

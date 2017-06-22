#ifndef BOOST_TEST_HELPER_HH_
# define BOOST_TEST_HELPER_HH_

#define BOOST_CHECK_EQUAL_MSG(Got, Expected, Msg) \
  do {                                            \
  BOOST_CHECK_EQUAL(Got, Expected);               \
  if (Got != Expected)                            \
    std::cout << "  => " << Msg << std::endl;     \
  } while (0)

#endif /* !BOOST_TEST_HELPER_HH_ */

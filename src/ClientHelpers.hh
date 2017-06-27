#ifndef CLIENTHELPERS_HH_
# define CLIENTHELPERS_HH_

#define CHECK_TOPIC                                     \
  do {                                                  \
    if (topic.empty())                                  \
    {                                                   \
      std::cout << "Invalid topic!" << std::endl;       \
      return 2;                                         \
    }                                                   \
  } while (0)

#define CHECK_PARTITION                                 \
  do {                                                  \
    if (partition < 0)                                  \
    {                                                   \
      std::cout << "Invalid partition!" << std::endl;   \
      return 3;                                         \
    }                                                   \
  } while (0)

#define CHECK_ERROR(X, Code, Msg)                               \
  do {                                                          \
    if (!res.ok())                                              \
    {                                                           \
      std::cout << "Can't " X ": " << res.error_code()          \
                << ": " << res.error_message() << std::endl;    \
      return 5;                                                 \
    }                                                           \
    if (Code != mykafka::Error::OK)                             \
    {                                                           \
      std::cout << Code                                         \
                << ": " << Msg << std::endl;                    \
      return 6;                                                 \
    }                                                           \
  } while (0)                                                   \

#endif /* !CLIENTHELPERS_HH_ */

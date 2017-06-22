#ifndef COMMIT_LOG_UTILS_HH_
# define COMMIT_LOG_UTILS_HH_

# include <inttypes.h>
# include "mykafka.pb.h"


namespace CommitLog
{
  namespace Utils
  {
    /*!
    ** Round down to the nearest multiple.
    ** Ex: (238, 8) => 232
    **
    ** @param value The value to round.
    ** @param factor The factor.
    **
    ** @return Rounded value.
    */
    int64_t roundDownToMultiple(int64_t value, int64_t factor);

    /*!
    ** Help to create an error message from a code and a message.
    **
    ** @param code The error code.
    ** @param msg A description of the error.
    **
    ** @return A custom error.
    */
    mykafka::Error err(mykafka::Error_ErrCode code, const std::string& msg);

    /*!
    ** Help to create an error message and deduce message from its error code.
    **
    ** @param code The error code.
    ** @param msg A description of the error.
    **
    ** @return A custom error.
    */
    mykafka::Error err(mykafka::Error_ErrCode code);
  }
}

#endif /* !COMMIT_LOG_UTILS_HH_ */

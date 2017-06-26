#include "utils/Utils.hh"

namespace Utils
{
  int64_t roundDownToMultiple(int64_t value, int64_t factor)
  {
    return value - (value % factor);
  }

  mykafka::Error err(mykafka::Error_ErrCode code, const std::string& msg)
  {
    mykafka::Error error;
    error.set_code(code);
    error.set_msg(msg);
    return error;
  }

  mykafka::Error err(mykafka::Error_ErrCode code)
  {
    const std::string msg = mykafka::Error_ErrCode_Name(code);
    return err(code, msg);
  }
} // Utils

#ifndef COMMIT_LOG_PARTITION_HH_
# define COMMIT_LOG_PARTITION_HH_

# include <boost/thread/shared_mutex.hpp>

# include "commitlog/Segment.hh"
# include "mykafka.pb.h"

namespace CommitLog
{
  /*!
  ** @class Partition
  **
  ** 4 byte CRC32 of the message
  ** 1 byte "magic" identifier which is always 0.
  ** 1 byte "attributes" which is always 0.
  ** 4 byte key length, containing length K
  ** K byte key
  ** 4 byte payload length, containing length V
  ** V byte payload
  **
  ** Size is: 4 + 1 + 1 + 4 + K + 4 + V = K + V + 14
  */
  class Partition
  {
  public:
    Partition();
    ~Partition();

    // FIXME add clean

  private:
    int64_t max_segment_size_;
    int64_t max_partition_size_;
    Segment* active_segment_;
    std::string path_;
    std::string name_;
    std::list<Segment*> segments_;
  };
} // CommitLog

#endif /* !COMMIT_LOG_PARTITION_HH_ */

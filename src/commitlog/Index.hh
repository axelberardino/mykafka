#ifndef COMMIT_LOG_INDEX_HH_
# define COMMIT_LOG_INDEX_HH_

# include <inttypes.h>
# include <string>
# include <mutex>

# include "mykafka.pb.h"

namespace CommitLog
{
  class Index
  {
  public:
    static const uint64_t OFFSET_WIDTH  = 4;
    static const uint64_t OFFSET_OFFSET = 0;
    static const uint64_t POSITION_WIDTH  = 4;
    static const uint64_t POSITION_OFFSET = OFFSET_WIDTH;
    static const uint64_t ENTRY_WIDTH = OFFSET_WIDTH + POSITION_WIDTH;

  public:

    Index();
    ~Index();

    mykafka::Error create();

  private:
    struct Entry
    {
      Entry();
      uint64_t offset_;
      uint64_t position_;
    };

  private:
    int64_t bytes_;
    int64_t base_offset_;
    int64_t position_;
    int fd_;
    void* addr_;
    std::string filename_;
    std::mutex mutex_;
  };
} // CommitLog


#endif /* !COMMIT_LOG_INDEX_HH_ */

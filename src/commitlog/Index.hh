#ifndef COMMIT_LOG_INDEX_HH_
# define COMMIT_LOG_INDEX_HH_

# include <inttypes.h>
# include <string>
# include <mutex>

# include <boost/thread/shared_mutex.hpp>

# include "mykafka.pb.h"

namespace CommitLog
{
  /*!
  ** @class Index
  **
  ** Store information about segments.
  **
  ** Example:
  **      001.index                       001.log
  **  offset, position        offset, position, size, payload
  **       0,        0             0,        0,    5, "first"
  **       1,        5             1,        5,    4, "test"
  **       2,        9             2,        9,   20, "{my_payload:content}"
  **       3,       29             3,       29,    2, "xx"
  */
  class Index
  {
  public:
    static const uint64_t DEFAULT_SIZE  = 10 * 1024 * 1024;
    static const uint64_t OFFSET_WIDTH  = 4;
    static const uint64_t OFFSET_OFFSET = 0;
    static const uint64_t POSITION_WIDTH  = 4;
    static const uint64_t POSITION_OFFSET = OFFSET_WIDTH;
    static const uint64_t ENTRY_WIDTH = OFFSET_WIDTH + POSITION_WIDTH;

  public:
    Index();
    ~Index();

    /*!
    ** Create a new index.
    **
    ** Open a new file (or an existing one).
    ** Resize it to a defined size before mmap'ing it.
    **
    ** @param filename The file name.
    ** @param bytes The bytes
    ** @param base_offset The base offset.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error create(const std::string& filename, int64_t bytes, int64_t base_offset);

    /*!
    ** Write a new entry into the index.
    **
    ** @param offset The offset.
    ** @param position The position.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error write(int64_t offset, int64_t position);

  private:
    // struct Entry
    // {
    //   Entry();
    //   uint64_t offset_;
    //   uint64_t position_;
    // };

  private:
    int64_t bytes_;
    int64_t base_offset_;
    int64_t position_;
    int fd_;
    int64_t* addr_;
    std::string filename_;
    boost::shared_mutex mutex_;
  };
} // CommitLog


#endif /* !COMMIT_LOG_INDEX_HH_ */

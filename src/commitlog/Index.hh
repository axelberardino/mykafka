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
    static const int64_t DEFAULT_SIZE  = 10 * 1024 * 1024;
    static const int64_t OFFSET_WIDTH  = 4;
    static const int64_t OFFSET_OFFSET = 0;
    static const int64_t POSITION_WIDTH  = 4;
    static const int64_t POSITION_OFFSET = OFFSET_WIDTH;
    static const int64_t ENTRY_WIDTH = OFFSET_WIDTH + POSITION_WIDTH;

  public:
    /*!
    ** Initialise a new index.
    **
    ** @param filename The file name.
    ** @param bytes The bytes
    ** @param base_offset The base offset.
    */
    Index(const std::string& filename, int64_t base_offset, int64_t bytes);

    /*!
    ** Close all files own.
    */
    ~Index();

    /*!
    ** Create a new index.
    **
    ** Open a new file (or an existing one).
    ** Resize it to a defined size before mmap'ing it.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error create();

    /*!
    ** Write a new entry into the index.
    **
    ** @param absolute_offset The offset (containing base_offset).
    ** @param position The position.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error write(int64_t absolute_offset, int64_t position);

    /*!
    ** Read an entry from the index.
    **
    ** @param rel_offset The offset to get.
    ** @param rel_position The position to get.
    ** @param relative_offset The offset (not containing base_offset).
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error read(int64_t& rel_offset, int64_t& rel_position,
                        int64_t relative_offset) const;

    /*!
    ** Force a file sync.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error sync();

    /*!
    ** Close the index file (also force sync and a resize).
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error close();

    /*!
    ** Check if index is corrupted.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error sanityCheck() const;

    /*!
    ** Get the file name of the index.
    **
    ** @return The file name.
    */
    std::string filename() const;

    /*!
    ** Truncate at number of entries.
    **
    ** @param nb The number where to truncate.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error truncateEntries(int64_t nb);

    /*!
    ** Get the file descriptor.
    **
    ** @return The file descriptor.
    */
    int fd() const;

    /*!
    ** Get the max size of the index.
    **
    ** @return The max size.
    */
    int64_t maxSize() const;

    /*!
    ** Get the internal base offset.
    **
    ** @return The base offset.
    */
    int64_t baseOffset() const;

    /*!
    ** Physically remove index file.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error deleteIndex();

  private:
    const int64_t size_;
    const int64_t base_offset_;
  public:
    int64_t position_;
  private:
    int fd_;
    void* addr_;
    const std::string filename_;
    mutable boost::shared_mutex mutex_;
  };
} // CommitLog


#endif /* !COMMIT_LOG_INDEX_HH_ */

#ifndef COMMIT_LOG_SEGMENT_HH_
# define COMMIT_LOG_SEGMENT_HH_

# include <boost/thread/mutex.hpp>

# include "mykafka.pb.h"

# include "commitlog/Index.hh"

namespace CommitLog
{
  class Segment
  {
  public:
    /*!
    ** Initialise a new segment.
    **
    ** @param filename The file name.
    ** @param base_offset The base offset.
    ** @param max_size The max size for this segment.
    */
    Segment(const std::string& filename, int64_t base_offset, int64_t max_size);

    /*!
    ** Close all files own.
    */
    ~Segment();

    /*!
    ** Create a new segment.
    **
    ** Create internal index file.
    ** Create a log file.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error create();

    /*!
    ** Check if segment is full
    **
    ** @return Segment is full.
    */
    bool isFull() const;

    /*!
    ** Close the segment file (also close the internal index file).
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error close();

    /*!
    ** Physically remove log and index files.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error deleteSegment();

    /*!
    ** Try to find an entry at a given offset.
    **
    ** @param rel_offset The offset of the entry.
    ** @param rel_position The position of the entry.
    ** @param search_offset The offset to search.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error findEntry(int64_t& rel_offset, int64_t& rel_position, int64_t search_offset) const;

  private:
    Index index_;
    int fd_;
    int64_t next_offset_;
    int64_t position_;
    const int64_t max_size_;
    std::string filename_;
    mutable boost::mutex mutex_;
  };
} // CommitLog

#endif /* !COMMIT_LOG_SEGMENT_HH_ */

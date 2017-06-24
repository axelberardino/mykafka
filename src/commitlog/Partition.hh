#ifndef COMMIT_LOG_PARTITION_HH_
# define COMMIT_LOG_PARTITION_HH_

# include "commitlog/Segment.hh"
# include "mykafka.pb.h"

# include <boost/thread/shared_mutex.hpp>
# include <vector>
# include <atomic>

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
    /*!
    ** Initialize a new partition.
    **
    ** @param path The path to the partition.
    ** @param max_segment_size The max size per segments.
    ** @param max_partition_size The max size of this
    **          partition (0 = no size restriction).
    ** @param segment_ttl Time after a segment has to
    **          be destroyed (0 = disabled).
    */
    Partition(const std::string& path, int64_t max_segment_size,
              int64_t max_partition_size, int64_t segment_ttl);

    /*!
    ** Close all files own and free segments.
    */
    ~Partition();

    /*!
    ** Open existing partition or create a new one.
    ** Create all directories needed. Reload existing segments
    ** and create new one if necessary.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error open();

    /*!
    ** Write payload into the right segments.
    ** If a new segment is created, also clean old segments.
    **
    ** @param payload Data to write.
    ** @param offset Where the data has been written.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error write(const std::vector<char>& payload, int64_t& offset);

    /*!
    ** Find the right segment, and then read data from it, at the right position.
    **
    ** @param payload Data to write.
    ** @param offset Where the data has been written.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error readAt(std::vector<char>& payload, int64_t offset);

    /*!
    ** Get the newest offset of the partition.
    **
    ** @return The newest offset.
    */
    int64_t newestOffset() const;

    /*!
    ** Get the oldest offset of the partition.
    **
    ** @return The oldest offset.
    */
    int64_t oldestOffset() const;

    /*!
    ** Get the current segment.
    **
    ** @return The active segment.
    */
    Segment* activeSegment() const;

    /*!
    ** Get an approximate physical size of the partition.
    **
    ** @return The physical size.
    */
    int64_t physicalSize() const;

    /*!
    ** Close all segments, and empty the segment
    ** list (free'ing all segments).
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error close();

    /*!
    ** Physically delete this partition and close all segments.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error deletePartition();

  private:
    /*!
    ** Remove old segments (either regarding size or timestamp).
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error cleanOldSegments();

    /*!
    ** Search for a segment containing offset.
    ** Try to find the closest segment where search_offset can be.
    **
    ** @param found_segment The founded_segment or 0 if not found.
    ** @param search_offset The offset.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error findSegment(Segment*& found_segment, int64_t search_offset);

  private:
    int64_t max_segment_size_;
    int64_t max_partition_size_;
    int64_t segment_ttl_;
    int64_t physical_size_;
    std::atomic<Segment*> active_segment_;
    std::string path_;
    std::string name_;
    std::vector<Segment*> segments_;
    mutable boost::shared_mutex mutex_;
  };
} // CommitLog

#endif /* !COMMIT_LOG_PARTITION_HH_ */

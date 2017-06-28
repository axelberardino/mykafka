#ifndef COMMIT_LOG_SEGMENT_HH_
# define COMMIT_LOG_SEGMENT_HH_

# include <vector>

# include "mykafka.pb.h"
# include "commitlog/Index.hh"

namespace CommitLog
{
  /*!
  ** @class Segment
  **
  ** A segment contains a log file and its index file.
  ** @see Index for index internal structure.
  **
  ** Log file has this structure:
  ** @verbatim
  **
  ** 8 byte offset
  ** 4 byte size of payload
  ** N byte for payload
  **
  ** So a message size is: 12 (header length) + N
  ** @verbatim
  */
  class Segment
  {
  public:
    static const int64_t OFFSET_SIZE = 8;
    static const int64_t SIZE_SIZE = 4;
    static const int64_t HEADER_SIZE = OFFSET_SIZE + SIZE_SIZE;

  public:
    /*!
    ** Initialize a new segment.
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
    ** Open existing segment or create a new one.
    **
    ** Create internal index file.
    ** Create a log file.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error open();

    /*!
    ** Reconstruct index from its log.
    ** Ensure index and log are synced. Get the last offset.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error reconstructIndexAndGetLastOffset();

    /*!
    ** Write at the end of the segment, the given payload.
    ** Will update the index as well.
    **
    ** @param payload The payload to append.
    ** @param payload_size The payload size.
    ** @param offset The offset where the data has been written.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error write(const char* payload, int32_t payload_size, int64_t& offset);
    mykafka::Error write(const std::vector<char>& payload, int64_t& offset);

    /*!
    ** Read segment at the specified.
    **
    ** @param payload The payload to append.
    ** @param relative_offset The offset to read.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error readAt(std::vector<char>& payload, int64_t relative_offset);

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
    ** Try to find an entry at a given offset (using a binary search).
    ** If no offset is found, then rel_offset value will be -1.
    **
    ** @param rel_offset The offset of the entry.
    ** @param rel_position The position of the entry.
    ** @param search_offset The offset to search.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error findEntry(int64_t& rel_offset, int64_t& rel_position,
                             int64_t search_offset) const;

    /*!
    ** Get the file descriptor.
    **
    ** @return The file descriptor.
    */
    int segmentFd() const;

    /*!
    ** Get the file descriptor.
    **
    ** @return The file descriptor.
    */
    int indexFd() const;

    /*!
    ** Get the next offset.
    **
    ** @return The next offset.
    */
    int64_t nextOffset() const;

    /*!
    ** Get the base offset.
    **
    ** @return The base offset.
    */
    int64_t baseOffset() const;

    /*!
    ** Dump the entire segment into a stream.
    **
    ** @param out The output stream.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error dump(std::ostream& out) const;

    /*!
    ** Get the physical size of this segment.
    **
    ** @return The physical size.
    */
    int64_t size() const;

    /*!
    ** Get the last time the segment file has been modified.
    **
    ** @return Last time modified.
    */
    int64_t mtime() const;

  private:
    struct Entry
    {
      int64_t offset;
      int32_t size;
    } __attribute__((packed));

  private:
    int fd_;
    int fd_read_;
    int64_t next_offset_;
    int64_t position_;
    int64_t physical_size_;
    int64_t mtime_;
    const int64_t max_size_;
    std::string filename_;
    Index index_;
  };
} // CommitLog

#endif /* !COMMIT_LOG_SEGMENT_HH_ */

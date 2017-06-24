#include "commitlog/Segment.hh"
#include "commitlog/Index.hh"
#include "commitlog/Utils.hh"

#include <linux/limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>

namespace CommitLog
{
  namespace
  {
    std::string getIndexFilename(const std::string& path, int64_t base_offset)
    {
      char buffer[PATH_MAX] = {0};
      sprintf(buffer, "%s/%020" PRId64 ".index", path.c_str(), base_offset);
      return std::string(buffer);
    }

    std::string getLogFilename(const std::string& path, int64_t base_offset)
    {
      char buffer[PATH_MAX] = {0};
      sprintf(buffer, "%s/%020" PRId64 ".log", path.c_str(), base_offset);
      return std::string(buffer);
    }
  } // namespace

  Segment::Segment(const std::string& filename, int64_t base_offset, int64_t max_size)
    : fd_(-1), next_offset_(base_offset), position_(0), max_size_(max_size),
      filename_(getLogFilename(filename, base_offset)),
      index_(getIndexFilename(filename, base_offset), base_offset, 0 /* use default size */),
      mutex_()
  {
    assert(sizeof (Entry) == HEADER_SIZE);
  }

  Segment::~Segment()
  {
    close();
  }

  mykafka::Error
  Segment::open()
  {
    auto res = index_.open();
    if (res.code() != mykafka::Error::OK)
      return res;

    res = index_.sanityCheck(); // Still useful ?
    if (res.code() != mykafka::Error::OK)
      return res;

    res = index_.truncateEntries(0);
    if (res.code() != mykafka::Error::OK)
      return res;

    fd_ = ::open(filename_.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd_ < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't"
                        " open log " + filename_ + "!");

    fd_read_ = ::open(filename_.c_str(), O_RDONLY, 0666);
    if (fd_read_ < 0)
    {
      if (::close(fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close file after a failed"
                          " read-only open log " + filename_ + "!");
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't"
                        " open read-only log " + filename_ + "!");
    }

    res = reconstructIndexAndGetLastOffset();
    if (res.code() != mykafka::Error::OK)
    {
      if (::close(fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close file after a failed"
                          " index reconstruct " + filename_ + "!");
      if (::close(fd_read_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close read-only file after a failed"
                          " index reconstruct " + filename_ + "!");
      return res;
    }

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Segment::reconstructIndexAndGetLastOffset()
  {
    if (::lseek(fd_, 0, SEEK_SET) < 0)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't seek"
                        " at start of log " + filename_ + "!");

    while (true)
    {
      auto bytes = ::read(fd_, &next_offset_, OFFSET_SIZE);
      if (bytes < 0)
        return Utils::err(mykafka::Error::LOG_ERROR, "Can't read offset"
                          " from log " + filename_ + "!");
      // End of file
      if (bytes == 0)
        break;

      int32_t size = -1;
      bytes = ::read(fd_, &size, SIZE_SIZE);
      if (bytes <= 0)
        return Utils::err(mykafka::Error::LOG_ERROR, "Can't read size"
                          " from log " + filename_ + "!");

      auto res = index_.write(next_offset_, position_);
      if (res.code() != mykafka::Error::OK)
        return res;

      position_ += size + HEADER_SIZE;
      ++next_offset_;

      if (::lseek(fd_, size, SEEK_CUR) < 0)
        break;
    }

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Segment::write(const char* payload, int32_t payload_size, int64_t& offset)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    const Entry entry{next_offset_, payload_size};
    if (::write(fd_, &entry, HEADER_SIZE) != HEADER_SIZE)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't write payload"
                        " offset/size to log " + filename_ + "!");
    if (::write(fd_, payload, payload_size) != payload_size)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't write payload"
                        " to log " + filename_ + "!");

    auto res  = index_.write(next_offset_, position_);
    if (res.code() != mykafka::Error::OK)
      return res;

    offset = next_offset_;
    ++next_offset_;
    position_ += HEADER_SIZE + payload_size;

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Segment::write(const std::string& payload, int64_t& offset)
  {
    return write(payload.data(), payload.size(), offset);
  }

  mykafka::Error
  Segment::readAt(std::vector<char>& payload, int64_t relative_offset)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    int64_t rel_offset = -1;
    int64_t rel_position = -1;
    auto res = findEntry(rel_offset, rel_position, relative_offset);
    if (res.code() != mykafka::Error::OK)
      return res;
    if (rel_offset == -1 || rel_position == -1)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't find offset " +
                        std::to_string(relative_offset) +
                        " when reading log " + filename_ + "!");

    if (::lseek(fd_read_, rel_position, SEEK_SET) < 0)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't seek at " +
                        std::to_string(rel_position) +
                        " in payload when reading " + filename_ + "!");

    Entry entry{-1, -1};
    if (::read(fd_read_, &entry, HEADER_SIZE) < 0 || entry.offset < 0 || entry.size < 0)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't read offset/size "
                        "from log " + filename_ + "!");

    payload.resize(entry.size);
    auto bytes = ::read(fd_read_, &payload[0], entry.size);
    if (bytes != entry.size)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't read payload "
                        "from log " + filename_ + "! (" +
                        std::to_string(bytes) + " != " + std::to_string(entry.size) + ")");

    return Utils::err(mykafka::Error::OK);
  }

  bool
  Segment::isFull() const
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    return position_ > max_size_;
  }

  mykafka::Error
  Segment::close()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (::close(fd_) < 0)
      return Utils::err(mykafka::Error::FILE_ERROR,
                        "Can't close log file " + filename_ + "!");
    if (::close(fd_read_) < 0)
      return Utils::err(mykafka::Error::FILE_ERROR,
                        "Can't close read-only log file " + filename_ + "!");

    fd_ = -1;
    fd_read_ = -1;
    position_ = 0;
    return index_.close();
  }

  mykafka::Error
  Segment::deleteSegment()
  {
    close();
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (::unlink(filename_.c_str()) < 0)
      return Utils::err(mykafka::Error::FILE_ERROR,
                        "Can't delete log file " + filename_ + "!");

    return index_.deleteIndex();
  }

  mykafka::Error
  Segment::findEntry(int64_t& rel_offset, int64_t& rel_position, int64_t search_offset) const
  {
    int64_t begin = 0;
    int64_t end = (next_offset_ - index_.baseOffset()) - 1;
    int64_t pos = (begin + end) / 2;
    rel_offset = -1;

    while (begin <= end && rel_offset != search_offset)
    {
      auto res = index_.read(rel_offset, rel_position, pos * CommitLog::Index::ENTRY_WIDTH);
      rel_offset -= index_.baseOffset();
      if (res.code() != mykafka::Error::OK)
        return res;
      if (rel_offset > search_offset)
        end = pos - 1;
      else
        begin = pos + 1;
      pos = (begin + end) / 2;
    }

    if (rel_offset != search_offset)
    {
      rel_offset = -1;
      rel_position = -1;
    }

    return Utils::err(mykafka::Error::OK);
  }

  int
  Segment::segmentFd() const
  {
    return fd_;
  }

  int
  Segment::indexFd() const
  {
    return index_.fd();
  }

  int64_t
  Segment::nextOffset() const
  {
    return next_offset_;
  }

  int64_t
  Segment::baseOffset() const
  {
    return index_.baseOffset();
  }

  mykafka::Error
  Segment::dump(std::ostream& out) const
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    int64_t rel_offset = -1;
    int64_t rel_position = -1;
    for (int64_t offset = 0; offset < (next_offset_ - index_.baseOffset()); ++offset)
    {
      auto res = index_.read(rel_offset, rel_position, offset * CommitLog::Index::ENTRY_WIDTH);
      if (res.code() != mykafka::Error::OK)
        return res;
      out << "For offset " << offset << ", using rel_offset=" << rel_offset
          << " and rel_position=" << rel_position << "\n";

      if (::lseek(fd_read_, rel_position, SEEK_SET) < 0)
        return Utils::err(mykafka::Error::LOG_ERROR, "Can't seek at " +
                          std::to_string(rel_position) +
                          " in payload when reading " + filename_ + "!");
      Entry entry{-1, -1};
      if (::read(fd_read_, &entry, HEADER_SIZE) < 0 || entry.offset < 0 || entry.size < 0)
        return Utils::err(mykafka::Error::LOG_ERROR, "Can't read offset/size "
                          "from log " + filename_ + "!");
      std::vector<char> payload;
      payload.resize(entry.size);
      auto bytes = ::read(fd_read_, &payload[0], entry.size);
      if (bytes != entry.size)
        return Utils::err(mykafka::Error::LOG_ERROR, "Can't read payload "
                          "from log " + filename_ + "! (" +
                          std::to_string(bytes) + " != " + std::to_string(entry.size) + ")");
      out << "=> | off: " << entry.offset << " | size: "
          << entry.size << " | payload: ";
      for (char c : payload)
        out << c;
      out << "\n";
    }

    return Utils::err(mykafka::Error::OK);
  }
} // CommitLog

#include "commitlog/Segment.hh"
#include "commitlog/Index.hh"
#include "commitlog/Utils.hh"

#include <linux/limits.h>
#include <unistd.h>
#include <fcntl.h>

namespace CommitLog
{
  namespace
  {
    std::string getIndexFilename(const std::string& path, int64_t base_offset)
    {
      char buffer[PATH_MAX] = {0};
      sprintf(buffer, "%s%020" PRId64 ".index", path.c_str(), base_offset);
      return std::string(buffer);
    }

    std::string getLogFilename(const std::string& path, int64_t base_offset)
    {
      char buffer[PATH_MAX] = {0};
      sprintf(buffer, "%s%020" PRId64 ".log", path.c_str(), base_offset);
      return std::string(buffer);
    }
  } // namespace

  Segment::Segment(const std::string& filename, int64_t base_offset, int64_t max_size)
    : index_(getIndexFilename(filename, base_offset), 0, base_offset),
      fd_(-1), next_offset_(base_offset), position_(0), max_size_(max_size),
      filename_(getLogFilename(filename, base_offset)), mutex_()
  {
  }

  Segment::~Segment()
  {
    close();
  }

  mykafka::Error
  Segment::create()
  {
    auto res = index_.create();
    if (res.code() != mykafka::Error::OK)
      return res;

    res = index_.sanityCheck();
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

    char buffer[16] = {0};
    while (true)
    {
      auto bytes = ::read(fd_, buffer, OFFSET_SIZE);
      if (bytes < 0)
        return Utils::err(mykafka::Error::LOG_ERROR, "Can't read offset"
                          " from log " + filename_ + "!");
      // End of file
      if (bytes == 0)
        break;

      next_offset_ = strtoll(buffer, 0, 10);

      bytes = ::read(fd_, buffer, SIZE_SIZE);
      if (bytes < 0)
        return Utils::err(mykafka::Error::LOG_ERROR, "Can't read size"
                          " from log " + filename_ + "!");
      // End of file
      if (bytes == 0)
        break;

      const int32_t size = strtoll(buffer, 0, 10);

      auto res = index_.write(next_offset_, position_);
      if (res.code() != mykafka::Error::OK)
        return res;

      std::cout << "my_pos: " << position_ << ", next_offset: "
                << next_offset_ << std::endl;

      position_ = size + HEADER_SIZE;
      ++next_offset_;

      if (::lseek(fd_, size, SEEK_CUR) < 0)
        break;
    }

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Segment::write(const char* payload, int32_t payload_size)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    // FIXME write struct
    if (::write(fd_, &next_offset_, sizeof(next_offset_)) != sizeof(next_offset_))
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't write payload"
                        " offset to log " + filename_ + "!");
    if (::write(fd_, &payload_size, sizeof(payload_size)) != sizeof(payload_size))
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't write payload"
                        " size to log " + filename_ + "!");
    if (::write(fd_, payload, payload_size) != payload_size)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't write payload"
                        " to log " + filename_ + "!");

    auto res  = index_.write(next_offset_, position_);
    if (res.code() != mykafka::Error::OK)
      return res;

    ++next_offset_;
    position_ += HEADER_SIZE + payload_size;

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Segment::write(const std::string& payload)
  {
    return write(payload.data(), payload.size());
  }

  mykafka::Error
  Segment::readAt(std::string& payload, int64_t offset)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    int64_t rel_offset = -1;
    int64_t rel_position = -1;
    auto res = findEntry(rel_offset, rel_position, offset);
    if (res.code() != mykafka::Error::OK)
      return res;

    if (::lseek(fd_read_, rel_position, SEEK_CUR) < 0)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't seek to "
                        "payload when reading " + filename_ + "!");

    int64_t got_offset = -1;
    int32_t got_size = -1;
    if (::read(fd_read_, &got_offset, sizeof(got_offset)) < 0 || got_offset < 0)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't read offset "
                        "from log " + filename_ + "!");
    if (::read(fd_read_, &got_size, sizeof(got_size)) < 0 || got_size < 0)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't read size "
                        "from log " + filename_ + "!");

    auto buffer = std::make_shared<char>(got_size);
    auto bytes = ::read(fd_read_, buffer.get(), got_size);
    if (bytes != got_size)
      return Utils::err(mykafka::Error::LOG_ERROR, "Can't read payload "
                        "from log " + filename_ + "!");

    payload = std::string(buffer.get());

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
    boost::lock_guard<boost::mutex> lock(mutex_);

    int64_t begin = 0;
    int64_t end = next_offset_ - 1;
    int64_t pos = (begin + end) / 2;
    rel_offset = -1;

    while (begin < end && rel_offset != search_offset)
    {
      auto res = index_.read(rel_offset, rel_position, pos * CommitLog::Index::ENTRY_WIDTH);
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
} // CommitLog

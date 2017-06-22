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
    : index_(getIndexFilename(filename, base_offset), 0 /* FIXME max_size ?*/, base_offset),
      fd_(0), next_offset_(0), position_(0), max_size_(max_size),
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

    res = index_.truncateEntries(0); // FIXME maybe not ?
    if (res.code() != mykafka::Error::OK)
      return res;

    fd_ = ::open(filename_.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd_ < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't open log " + filename_ + "!");

    return Utils::err(mykafka::Error::OK);
  }

  bool
  Segment::isFull() const
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    return position_ >  max_size_;
  }

  mykafka::Error
  Segment::close()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (::close(fd_) < 0)
      return Utils::err(mykafka::Error::FILE_ERROR,
                        "Can't close log file " + filename_ + "!");

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
    // binary search

    auto res = index_.read(rel_offset, rel_position, search_offset);
    if (res.code() != mykafka::Error::OK)
      return res;

    return Utils::err(mykafka::Error::OK);
  }
} // CommitLog

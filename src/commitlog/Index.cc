#include "commitlog/Index.hh"
#include "commitlog/Utils.hh"

#include <boost/thread/locks.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace CommitLog
{
  namespace
  {
    inline int32_t relativeOffset(int64_t offset, int64_t base_offset)
    {
      return offset - base_offset;
    }
  } // namespace

  Index::Index()
    : bytes_(0), base_offset_(0), position_(0),
      fd_(0), addr_(0), filename_(), mutex_()
  {
  }

  Index::~Index()
  {
  }

  mykafka::Error
  Index::create(const std::string& filename, int64_t bytes, int64_t base_offset)
  {
    bytes_ = bytes != 0 ? bytes : DEFAULT_SIZE;
    filename_ = filename;
    base_offset_ =  base_offset;

    mykafka::Error err;
    if (filename_.empty())
      return Utils::err(mykafka::Error::INVALID_FILENAME, "Empty index filename given!");

    fd_ = open(filename_.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd_ < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't open index " + filename_ + "!");

    struct stat buf;
    if (fstat(fd_, &buf) < 0)
    {
      close(fd_);
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't stat index " + filename_ + "!");
    }

    position_ = buf.st_size;
    if (ftruncate(fd_, Utils::roundDownToMultiple(bytes_, ENTRY_WIDTH) < 0))
    {
      close(fd_);
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't resize index " + filename_ + "!");
    }

    addr_ = static_cast<int64_t*>(::mmap(0, buf.st_size, PROT_READ | PROT_WRITE,
                                         MAP_SHARED, fd_, 0 /*position ?*/));
    if (addr_ == MAP_FAILED)
    {
      close(fd_);
      return Utils::err(mykafka::Error::FILE_ERROR, "Error mapping the file " + filename_ + "!");
    }

    return err;
  }

  mykafka::Error
  Index::write(int64_t offset, int64_t position)
  {
    const int32_t rel_offset = relativeOffset(offset, base_offset_);
    const int32_t rel_position = position;
    boost::lock_guard<boost::shared_mutex> lock(mutex_);
    *addr_ = rel_offset;
    ++addr_;
    *addr_ = rel_position;
    ++addr_;
    position_ += ENTRY_WIDTH;
  }


} // CommitLog

#include "commitlog/Index.hh"
#include "utils/Utils.hh"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstring>

namespace CommitLog
{
  Index::Index(const std::string& filename, int64_t base_offset, int64_t size)
    : size_(size != 0 ? size : DEFAULT_SIZE), base_offset_(base_offset), position_(0),
      fd_(-1), addr_(0), filename_(filename)
  {
  }

  Index::~Index()
  {
    close();
  }

  mykafka::Error
  Index::open()
  {
    mykafka::Error err;
    if (filename_.empty())
      return Utils::err(mykafka::Error::INVALID_FILENAME, "Empty index filename given!");

    fd_ = ::open(filename_.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd_ < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't open index " +
                        filename_ + " because: " + std::string(::strerror(errno)));

    struct stat buf;
    if (::fstat(fd_, &buf) < 0)
    {
      if (::close(fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed stat index " +
                          filename_ + " because: " + std::string(::strerror(errno)));
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't stat index " +
                        filename_ + " because: " + std::string(::strerror(errno)));
    }

    position_ = buf.st_size;
    const int64_t rounded_size = Utils::roundDownToMultiple(size_, ENTRY_WIDTH);
    if (::ftruncate(fd_, rounded_size) < 0)
    {
      if (::close(fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed resize index " +
                          filename_ + " because: " + std::string(::strerror(errno)));
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't resize index " +
                        filename_ + " because: " + std::string(::strerror(errno)));
    }
    if (::lseek(fd_, rounded_size - 1, SEEK_SET) < 0)
    {
      if (::close(fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed seek index " +
                          filename_ + " because: " + std::string(::strerror(errno)));
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't seek index " +
                        filename_ + " because: " + std::string(::strerror(errno)));
    }
    if (::write(fd_, "", 1) < 0)
    {
      if (::close(fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed write 0 at the index end " +
                          filename_ + " because: " + std::string(::strerror(errno)));
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't write at index end " +
                        filename_ + " because: " + std::string(::strerror(errno)));
    }

    addr_ = ::mmap(0, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0 /*position ?*/);
    if (addr_ == MAP_FAILED)
    {
      if (::close(fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed mmap " +
                          filename_ + " because: " + std::string(::strerror(errno)));
      return Utils::err(mykafka::Error::FILE_ERROR, "Error mapping the file " +
                        filename_ + " because: " + std::string(::strerror(errno)));
    }

    return err;
  }

  mykafka::Error
  Index::write(int64_t absolute_offset, int64_t position)
  {
    const int32_t rel_offset = absolute_offset - base_offset_;
    const int32_t rel_position = position;

    if (position_ >= size_)
      return Utils::err(mykafka::Error::INDEX_ERROR, "Write overflow!"
                        " (" + std::to_string(position_) + " >= " +
                        std::to_string(size_) + ")");

    *reinterpret_cast<int32_t*>(static_cast<char*>(addr_) + position_) = rel_offset;
    position_ += OFFSET_WIDTH;
    *reinterpret_cast<int32_t*>(static_cast<char*>(addr_) + position_) = rel_position;
    position_ += POSITION_WIDTH;

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Index::read(int64_t& rel_offset, int64_t& rel_position, int64_t relative_offset) const
  {
    if (relative_offset > size_ - ENTRY_WIDTH)
      return Utils::err(mykafka::Error::INDEX_ERROR, "Read overflow!");

    rel_offset = *reinterpret_cast<int32_t*>(static_cast<char*>(addr_) +
                                             relative_offset) + base_offset_;
    rel_position = *reinterpret_cast<int32_t*>(static_cast<char*>(addr_)
                                               + relative_offset + OFFSET_WIDTH);

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Index::sync()
  {
    if (msync(addr_, position_, MS_SYNC) < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't msync " +
                        filename_ + " because: " + std::string(::strerror(errno)));
    if (syncfs(fd_) < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't file sync " +
                        filename_ + " because: " + std::string(::strerror(errno)));

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Index::close()
  {
    // Already closed!
    if (fd_ < 0)
      return Utils::err(mykafka::Error::OK);

    //    sync(); // FIXME : Cost a lost but could be mandatory.
    if (munmap(addr_, size_) < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't unmap index " +
                        filename_ + " because: " + std::string(::strerror(errno)));

    if (ftruncate(fd_, position_) < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't resize index " +
                        filename_ + " because: " + std::string(::strerror(errno)));

    if (::close(fd_))
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't close " +
                        filename_ + " because: " + std::string(::strerror(errno)));

    fd_ = -1;
    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Index::sanityCheck() const
  {
    if (position_ == 0)
      return Utils::err(mykafka::Error::OK);

    if (position_ % ENTRY_WIDTH != 0)
      return Utils::err(mykafka::Error::INDEX_ERROR,
                        "Index seems corrupted: position_ % ENTRY_WIDTH");

    int64_t rel_offset = 0;
    int64_t rel_position = 0;
    auto res = read(rel_offset, rel_position, position_ - ENTRY_WIDTH);
    if (res.code() != mykafka::Error::OK)
      return res;

    if (rel_offset < base_offset_)
      return Utils::err(mykafka::Error::INDEX_ERROR,
                        "Index seems corrupted: rel_offset < base_offset_");

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Index::truncateEntries(int64_t nb)
  {
    if ((nb * ENTRY_WIDTH) > position_)
      return Utils::err(mykafka::Error::INDEX_ERROR, "Invalid truncate number!");

    position_ = nb * ENTRY_WIDTH;

    return Utils::err(mykafka::Error::OK);
  }

  std::string
  Index::filename() const
  {
    return filename_;
  }

  int
  Index::fd() const
  {
    return fd_;
  }

  int64_t
  Index::baseOffset() const
  {
    return base_offset_;
  }

  mykafka::Error
  Index::deleteIndex()
  {
    if (::unlink(filename_.c_str()) < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't delete index file " +
                        filename_ + " because: " + std::string(::strerror(errno)));

    return Utils::err(mykafka::Error::OK);
  }
} // CommitLog

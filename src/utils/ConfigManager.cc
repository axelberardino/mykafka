#include "utils/ConfigManager.hh"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace Utils
{
  namespace fs = boost::filesystem;

  ConfigManager::ConfigManager(const std::string& path)
    : base_path_(path)
  {
  }

  ConfigManager::~ConfigManager()
  {
  }

  mykafka::Error
  ConfigManager::load()
  {
    for (auto& entry : boost::make_iterator_range(fs::directory_iterator(base_path_), {}))
    {
      if (fs::is_regular(entry) && entry.path().extension().string() == ".cfg")
      {
        const std::string pathame = entry.path().string();
        const std::string filename = entry.path().stem().string();

        // open({topic, partition});
        std::cout << pathame << filename << std::endl;
      }
    }

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  ConfigManager::create(const TopicPartition& key,
                        int64_t seg_size, int64_t part_size, int64_t ttl)
  {
    const std::string path = base_path_ + "/" + key.toString();
    if (fs::exists(path))
      return Utils::err(mykafka::Error::FILE_ERROR, "File already exists: " + path);

    CfgInfo info;
    info.fd_ = ::open(path.c_str(), O_RDWR | O_CREAT, 0666);
    if (info.fd_ < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't open config file " + path + "!");

    struct stat buf;
    if (::fstat(info.fd_, &buf) < 0)
    {
      if (::close(info.fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed stat config file " + path + "!");
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't stat config file " + path + "!");
    }

    const int64_t size = sizeof (RawInfo);
    if (::ftruncate(info.fd_, size) < 0)
    {
      if (::close(info.fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed resize config file " + path + "!");
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't resize config file " + path + "!");
    }
    if (::lseek(info.fd_, size - 1, SEEK_SET) < 0)
    {
      if (::close(info.fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed seek config file " + path + "!");
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't seek config file " + path + "!");
    }
    if (::write(info.fd_, "", 1) < 0)
    {
      if (::close(info.fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed write 0 at the config file end " + path + "!");
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't write at config file end " + path + "!");
    }

    info.addr_ = ::mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, info.fd_, 0);
    if (info.addr_ == MAP_FAILED)
    {
      if (::close(info.fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed mmap " + path + "!");
      return Utils::err(mykafka::Error::FILE_ERROR, "Error mapping the file " + path + "!");
    }

    *reinterpret_cast<RawInfo*>(info.addr_) = {seg_size, part_size, ttl, 0, 0};
    configs_.insert(std::make_pair(key, info));

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  ConfigManager::open(const TopicPartition& key)
  {
    const std::string path = base_path_ + "/" + key.toString();
    if (!fs::is_regular(path))
      return Utils::err(mykafka::Error::FILE_ERROR, "File not exists: " + path);

    CfgInfo info;
    info.fd_ = ::open(path.c_str(), O_RDWR | O_CREAT, 0666);
    if (info.fd_ < 0)
      return Utils::err(mykafka::Error::FILE_ERROR, "Can't open config file " + path + "!");

    const int64_t size = sizeof (RawInfo);
    info.addr_ = ::mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, info.fd_, 0);
    if (info.addr_ == MAP_FAILED)
    {
      if (::close(info.fd_) < 0)
        return Utils::err(mykafka::Error::FILE_ERROR,
                          "Can't close during failed mmap " + path + "!");
      return Utils::err(mykafka::Error::FILE_ERROR, "Error mapping the file " + path + "!");
    }

    info.info = *reinterpret_cast<RawInfo*>(info.addr_);
    configs_.insert(std::make_pair(key, info));

    return Utils::err(mykafka::Error::OK);
  }

  void
  ConfigManager::dump(std::ostream& out) const
  {
    for (auto& entry : configs_)
    {
      out << "Topic/partition:" << entry.first.toString()
          << ", fd: " << entry.second.fd_
          << ", max_seg_size: " << entry.second.info.max_segment_size
          << ", max_part_size: " << entry.second.info.max_partition_size
          << ", segment_ttl: " << entry.second.info.segment_ttl
          << ", reader_offset: " << entry.second.info.reader_offset
          << ", commit_offset: " << entry.second.info.commit_offset
          << std::endl;
    }
  }
} // Utils

#include "commitlog/Partition.hh"
#include "commitlog/Utils.hh"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <cassert>

namespace CommitLog
{
  namespace
  {
    int64_t detectBaseOffset(const std::string& s)
    {
      int64_t res = strtoll(s.c_str(), 0, 10);
      if (res == 0 && s.find_first_not_of("0") != std::string::npos)
        return -1;
      return res;
    }
  } // namespace

  Partition::Partition(const std::string& path, int64_t max_segment_size, int64_t max_partition_size)
    : max_segment_size_(max_segment_size), max_partition_size_(max_partition_size),
      active_segment_(0), path_(path), name_(), segments_()
  {
  }

  Partition::~Partition()
  {
    close();
  }

  mykafka::Error
  Partition::open()
  {
    namespace fs = boost::filesystem;

    fs::path raw_path(path_);
    try
    {
      raw_path = fs::absolute(raw_path);
      path_ = raw_path.string();
      name_ = raw_path.filename().string();

      fs::create_directories(raw_path);
      for (auto& entry : boost::make_iterator_range(fs::directory_iterator(raw_path), {}))
      {
        if (entry.path().extension().string() == "log")
        {
          int64_t base_offset = detectBaseOffset(entry.path().stem().string());
          if (base_offset < 0)
          {
            Segment* segment = new Segment(path_, base_offset, max_segment_size_);
            auto res = segment->open();
            if (res.code() != mykafka::Error::OK)
            {
              delete segment;
              return res;
            }
            segments_.push_back(segment);
          }
        }
      }

      if (segments_.empty())
      {
        Segment* segment = new Segment(path_, 0, max_segment_size_);
        auto res = segment->open();
        if (res.code() != mykafka::Error::OK)
        {
          delete segment;
          return res;
        }
        segments_.push_back(segment);
      }

      active_segment_ = segments_.back();
    }
    catch (fs::filesystem_error& e)
    {
      return Utils::err(mykafka::Error::INVALID_FILENAME, e.what());
    }

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Partition::write(const std::string& payload, int64_t& offset)
  {
    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Partition::read(std::vector<char>& payload)
  {
    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error readAt(std::vector<char>& payload, int64_t offset)
  {
    return Utils::err(mykafka::Error::OK);
  }

  int64_t
  Partition::newestOffset() const
  {
    assert(active_segment_);
    return (*active_segment_).nextOffset();
  }

  int64_t
  Partition::oldestOffset() const
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    return segments_.front()->baseOffset();
  }

  Segment*
  Partition::activeSegment() const
  {
    return active_segment_;
  }

  mykafka::Error
  Partition::close()
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);

    active_segment_ = 0;
    for (auto segment : segments_)
    {
      auto res = segment->close();
      if (res.code() != mykafka::Error::OK)
        return res;
      delete segment;
    }
    segments_.clear();

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Partition::deletePartition()
  {
    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Partition::truncate()
  {
    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Partition::cleanOldSegments()
  {
    return Utils::err(mykafka::Error::OK);
  }
} // CommitLog

#include "commitlog/Partition.hh"
#include "utils/Utils.hh"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <cassert>

namespace CommitLog
{
  namespace fs = boost::filesystem;

  namespace
  {
    int64_t detectBaseOffset(const std::string& s)
    {
      int64_t res = strtoll(s.c_str(), 0, 10);
      if (res == 0 && s.find_first_not_of("0") != std::string::npos)
        return -1;
      return res;
    }

    void fillFilesList(const fs::path& raw_path, std::vector<int64_t>& offset_list)
    {
      for (auto& entry : boost::make_iterator_range(fs::directory_iterator(raw_path), {}))
      {
        if (fs::is_regular(entry) && entry.path().extension().string() == ".log")
        {
          const int64_t base_offset = detectBaseOffset(entry.path().stem().string());
          if (base_offset >= 0)
            offset_list.push_back(base_offset);
        }
      }
      std::sort(offset_list.begin(), offset_list.end());
    }
  } // namespace

  Partition::Partition(const std::string& path, int64_t max_segment_size,
                       int64_t max_partition_size, int64_t segment_ttl)
    : cancel_(false), max_segment_size_(max_segment_size),
      max_partition_size_(max_partition_size), segment_ttl_(segment_ttl),
      physical_size_(0), active_segment_(0), path_(path), name_(), segments_()
  {
  }

  Partition::~Partition()
  {
    close();
  }

  mykafka::Error
  Partition::open()
  {
    fs::path raw_path(path_);
    try
    {
      raw_path = fs::absolute(raw_path);
      path_ = raw_path.string();
      name_ = raw_path.filename().string();

      fs::create_directories(raw_path);
      std::vector<int64_t> offset_list;
      fillFilesList(raw_path, offset_list);
      for (auto base_offset : offset_list)
      {
        Segment* segment = new Segment(path_, base_offset, max_segment_size_);
        auto res = segment->open();
        if (res.code() != mykafka::Error::OK)
        {
          delete segment;
          return res;
        }
        physical_size_ += segment->size();
        segments_.push_back(segment);
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

    boost::lock_guard<boost::shared_mutex> lock(mutex_);
    cleanOldSegments();
    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Partition::write(const std::vector<char>& payload, int64_t& offset)
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);
    if (cancel_)
      return Utils::err(mykafka::Error::PARTITION_ERROR, "Partition is closed");

    assert(active_segment_);
    if ((*active_segment_).isFull())
    {
      Segment* segment = new Segment(path_, (*active_segment_).nextOffset(),
                                     max_segment_size_);
      auto res = segment->open();
      if (res.code() != mykafka::Error::OK)
      {
        delete segment;
        return res;
      }
      segments_.push_back(segment);
      if (segment_ttl_ != 0 || max_partition_size_ != 0)
        cleanOldSegments(); // /!\ If segments are small, could destroy performance...
      active_segment_ = segments_.back();
    }

    auto res = (*active_segment_).write(payload, offset);
    if (res.code() != mykafka::Error::OK)
      return res;

    physical_size_ += payload.size() + Segment::HEADER_SIZE;

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Partition::readAt(std::vector<char>& payload, int64_t offset)
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);
    if (cancel_)
      return Utils::err(mykafka::Error::PARTITION_ERROR, "Partition is closed");

    Segment* found_segment = 0;
    auto res = findSegment(found_segment, offset);
    if (res.code() != mykafka::Error::OK)
      return res;
    if (!found_segment)
      return Utils::err(mykafka::Error::PARTITION_ERROR, "Can't find "
                        "segment for offset " + std::to_string(offset));

    res = found_segment->readAt(payload, offset - found_segment->baseOffset());
    if (res.code() != mykafka::Error::OK)
      return res;

    return Utils::err(mykafka::Error::OK);
  }

  int64_t
  Partition::newestOffset() const
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    if (cancel_)
      return -1;

    assert(active_segment_);
    return (*active_segment_).nextOffset();
  }

  int64_t
  Partition::oldestOffset() const
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    if (cancel_)
      return -1;

    return segments_.front()->baseOffset();
  }

  Segment*
  Partition::activeSegment() const
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    return active_segment_;
  }

  int64_t
  Partition::physicalSize() const
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    return physical_size_;
  }

  mykafka::Error
  Partition::close()
  {
    boost::lock_guard<boost::shared_mutex> lock(mutex_);

    cancel_ = true;
    active_segment_ = 0;
    physical_size_ = 0;
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
    auto res = close();
    if (res.code() != mykafka::Error::OK)
      return res;

    fs::remove_all(path_);

    return Utils::err(mykafka::Error::OK);
  }

  mykafka::Error
  Partition::cleanOldSegments()
  {
    auto res = Utils::err(mykafka::Error::OK);
    const int64_t now = ::time(0);
    const int64_t seg_ttl = segment_ttl_;
    const int64_t max_size = max_partition_size_;
    segments_.erase(std::remove_if(segments_.begin(), segments_.end(),
                                   [&](Segment* segment)
                                   {
                                     if (segment == active_segment_)
                                       return false;

                                     const int64_t size = segment->size();
                                     const int64_t ts = segment->mtime();
                                     const bool too_old = seg_ttl != 0 && now - ts > seg_ttl;
                                     const bool partition_too_large = max_size != 0 &&
                                       physical_size_ > max_size;
                                     if (too_old || partition_too_large)
                                     {
                                       auto local_res = segment->deleteSegment();
                                       if (local_res.code() != mykafka::Error::OK)
                                       {
                                         res = local_res;
                                         return false;
                                       }
                                       delete segment;
                                       physical_size_ -= size;
                                       return true;
                                     }
                                     return false;
                                   }), segments_.end());

    return res;
  }

  mykafka::Error
  Partition::findSegment(Segment*& found_segment, int64_t search_offset)
  {
    found_segment = 0;
    if (segments_.empty())
      return Utils::err(mykafka::Error::OK);

    int64_t begin = 0;
    int64_t end = segments_.size() - 1;
    int64_t pos = (begin + end) / 2;
    int64_t found_offset = -1;

    while (begin <= end && found_offset != search_offset)
    {
      found_offset = segments_[pos]->baseOffset();
      if (found_offset > search_offset)
        end = pos - 1;
      else
      {
        found_segment = segments_[pos];
        begin = pos + 1;
      }
      pos = (begin + end) / 2;
    }

    return Utils::err(mykafka::Error::OK);
  }
} // CommitLog

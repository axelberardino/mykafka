#ifndef UTILS_CONFIGMANAGER_HH_
# define UTILS_CONFIGMANAGER_HH_

# include "utils/Utils.hh"

# include <boost/functional/hash.hpp>
# include <unordered_map>

namespace Utils
{
  /*!
  ** @class ConfigManager
  **
  ** Use mmap to store information about partition.
  */
  class ConfigManager
  {
  public:
    struct TopicPartition
    {
      bool operator==(const TopicPartition& key) const
      {
        return topic == key.topic && partition == key.partition;
      }

      std::size_t hash_value() const
      {
        std::size_t seed = 0;
        boost::hash_combine(seed, topic);
        boost::hash_combine(seed, partition);
        return seed;
      }

      std::string toString() const
      {
        return topic + "-" + std::to_string(partition);
      }

      std::string topic;
      int32_t partition;
    };

    struct RawInfo
    {
      int64_t max_segment_size;
      int64_t max_partition_size;
      int64_t segment_ttl;
      int64_t reader_offset;
      int64_t commit_offset;
    } __attribute__((packed));

    struct CfgInfo
    {
      int fd_;
      void* addr_;
      RawInfo info;
    };

  public:
    ConfigManager(const std::string& path);
    ~ConfigManager();

    mykafka::Error load();
    mykafka::Error create(const TopicPartition& key,
                          int64_t seg_size, int64_t part_size, int64_t ttl);
    mykafka::Error open(const TopicPartition& key);
    mykafka::Error get(const TopicPartition& key, RawInfo& info) const;
    mykafka::Error update(const TopicPartition& key, const RawInfo& info);
    mykafka::Error remove(const TopicPartition& key);
    mykafka::Error close(const TopicPartition& key);
    mykafka::Error close();

    void dump(std::ostream& out) const;
    int32_t size() const;

  private:
    mykafka::Error close(const CfgInfo& info, const std::string& filename);

  private:
    const std::string base_path_;
    std::unordered_map<TopicPartition, CfgInfo, Hash<TopicPartition> > configs_;
  };
} // Utils

#endif /* !UTILS_CONFIGMANAGER_HH_ */

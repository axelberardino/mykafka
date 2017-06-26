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
    /*!
    ** @struct TopicPartition
    **
    ** Use to handle a key compatible with an unordered_map.
    */
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

  private:
    /*!
    ** @struct RawInfo
    **
    ** Data contained in the mmap file.
    */
    struct RawInfo
    {
      int64_t max_segment_size;
      int64_t max_partition_size;
      int64_t segment_ttl;
      int64_t reader_offset;
      int64_t commit_offset;
    } __attribute__((packed));

    /*!
    ** @struct CfgInfo
    **
    ** Wrapper which hold data and
    ** info about the mmap'ed file.
    */
    struct CfgInfo
    {
      int fd_;
      void* addr_;
      RawInfo info;
    };

  public:
    /*!
    ** Initialize a config manager.
    **
    ** @param path The base directory for config files.
    */
    ConfigManager(const std::string& path);

    /*!
    ** Will close all config files.
    */
    ~ConfigManager();

    /*!
    ** Load all config files (*.cfg) in the directory path.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error load();

    /*!
    ** Create a new config file.
    ** @warning Filename must not exists !
    **
    ** @param key The topic/partition key.
    ** @param seg_size The max segment size.
    ** @param part_size The max partition size.
    ** @param ttl The segment ttl.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error create(const TopicPartition& key,
                          int64_t seg_size, int64_t part_size, int64_t ttl);

    /*!
    ** Open an existing config file.
    ** @warning Filename must exists !
    **
    ** @param key The topic/partition key.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error open(const TopicPartition& key);

    /*!
    ** Get the content of a config file.
    **
    ** @param key The topic/partition key.
    ** @param info The content of the config file.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error get(const TopicPartition& key, RawInfo& info) const;

    /*!
    ** Update info about an existing config file.
    ** @warning Overwrite the entire file, info must be exhaustive.
    **
    ** @param key The topic/partition key.
    ** @param info The new content of the config file.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error update(const TopicPartition& key, const RawInfo& info);

    /*!
    ** Close the config file, erase it from topics list
    ** and physically remove it from disk.
    **
    ** @param key The topic/partition key.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error remove(const TopicPartition& key);

    /*!
    ** Close the config file, erase it from topics list.
    **
    ** @param key The topic/partition key.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error close(const TopicPartition& key);

    /*!
    ** Close all config files, and clear the topics list.
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error close();

    /*!
    ** Dump the content of the config manager into a stream.
    **
    ** @param out The given stream.
    */
    void dump(std::ostream& out) const;

    /*!
    ** Get the current size of the topics list.
    **
    ** @return The topics number.
    */
    int32_t size() const;

  private:
    /*!
    ** Helper method which unmap and close a file.
    **
    ** @param info The struct containing the fd and mmap address.
    ** @param filename Filename (only for nice error message).
    **
    ** @return Error code 0 if no error, or a detailed error.
    */
    mykafka::Error close(const CfgInfo& info, const std::string& filename);

  private:
    const std::string base_path_;
    std::unordered_map<TopicPartition, CfgInfo, Hash<TopicPartition> > configs_;
  };
} // Utils

#endif /* !UTILS_CONFIGMANAGER_HH_ */

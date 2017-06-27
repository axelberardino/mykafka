#include "broker/Broker.hh"
#include "utils/Utils.hh"
#include "boost_test_helper.hh"

#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <inttypes.h>
#include <thread>
#include <array>
#include <algorithm>
#include <atomic>
#include <iomanip>

namespace
{
  std::random_device rd;
  std::atomic<int64_t> nb_written{0};
  std::atomic<int64_t> bytes_written{0};
  std::atomic<int64_t> nb_read{0};
  std::atomic<int64_t> bytes_read{0};

  const std::string prettySize(uint64_t size)
  {
    static const uint64_t k1023 = 1023;
    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", 0};
    static const uint64_t masks[] = {k1023, k1023 << 10, k1023 << 20, k1023 << 30,
                                     k1023 << 40, k1023 << 50, k1023 << 60};

    std::ostringstream buff;
    int64_t unit;
    float sizef;
    for (unit = 6; !(size & masks[unit]) && unit; --unit)
      ;

    if (unit >= 1)
      sizef = ((float)(size >> ((unit - 1) * 10))) / 1024;
    else
      sizef = size;

    buff << std::setiosflags(std::ios::fixed) << std::setprecision(3)
         << sizef << " " << units[unit];

    return buff.str();
  }

  std::string randomString(size_t length)
  {
    auto randchar = []() -> char
      {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max = (sizeof(charset) - 1);
        std::mt19937 rng(rd());
        std::uniform_int_distribution<int> uni(0, max);
        return charset[uni(rng)];
      };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
  }

  std::string genData()
  {
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(0, 10000);
    return randomString(uni(rng));
  }

  void readFrom(Broker::Broker& broker, const std::string& topic,
                int32_t partition, int64_t offset, int64_t nb)
  {
    mykafka::GetMessageRequest request;
    request.set_topic(topic);
    request.set_partition(partition);
    request.set_offset(offset);
    mykafka::GetMessageResponse response;

    for (int i = 0; i < nb; ++i)
    {
      broker.getMessage(request, response);
      ++nb_read;
      bytes_read += response.payload().size();
    }
  }

  void writeFrom(Broker::Broker& broker, const std::string& topic,
                 int32_t partition, int64_t nb)
  {
    mykafka::SendMessageRequest request;
    request.set_topic(topic);
    request.set_partition(partition);
    request.set_payload(genData());
    mykafka::SendMessageResponse response;

    for (int i = 0; i < nb; ++i)
    {
      broker.sendMessage(request, response);
      ++nb_written;
      bytes_written += request.payload().size();
    }
  }
} // namespace

int main()
{
  namespace fs = boost::filesystem;

  const std::string tmp_path = "/tmp/mykafka-test/broker-bench/";
  const std::string topic = "bench_readwrite";
  const int32_t partition = 0;
  const int32_t nb_core = 8;
  const int32_t data_size = 2000000;
  const int32_t nb_start_data = data_size / nb_core;

  fs::remove_all(tmp_path);
  fs::create_directories(tmp_path);

  std::cout << "Start bench..." << std::endl;
  Broker::Broker broker(tmp_path);

  mykafka::TopicPartitionRequest request;
  request.set_topic(topic);
  request.set_max_segment_size(4096 * 1024 * 10); // 40 Mo segment size
  request.set_max_partition_size(4096 * 1024 * 100); // 400 Mo max partition size
  request.set_segment_ttl(0);
  request.set_partition(partition);
  broker.createPartition(request);

  auto start = std::chrono::system_clock::now();
  writeFrom(broker, topic, partition, nb_start_data);
  readFrom(broker, topic, partition, 0, nb_start_data);

  std::vector<std::thread> threads;
  for (int i = 0; i < nb_core; ++i)
  {
    threads.emplace_back(std::thread([&broker,topic,partition]() {
          writeFrom(broker, topic, partition, nb_start_data);
        }));
    threads.emplace_back(std::thread([&broker,topic,partition]() {
          readFrom(broker, topic, partition, 0, nb_start_data);
        }));
  }
  for (auto& thread : threads)
    thread.join();

  auto end = std::chrono::system_clock::now();
  const int64_t elapsed_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::cout << nb_written << " write in " << elapsed_ms << "ms"
            << " (" << (nb_written * 1000) / elapsed_ms << " msg/s)"
            << std::endl
            << "    " << prettySize(bytes_written) << " write in " << elapsed_ms << "ms"
            << " (" << prettySize((bytes_written * 1000) / elapsed_ms) << " bytes/s)"
            << std::endl
            << nb_read << " read in " << elapsed_ms << "ms"
            << " (" << (nb_read * 1000) / elapsed_ms << " msg/s)"
            << std::endl
            << "    " << prettySize(bytes_read) << " write in " << elapsed_ms << "ms"
            << " (" << prettySize((bytes_read * 1000) / elapsed_ms) << " bytes/s)"
            << std::endl
            << (nb_written + nb_read) << " operation in " << elapsed_ms << "ms"
            << " (" << ((nb_written + nb_read) * 1000 / elapsed_ms) << " msg/s)"
            << std::endl
            << "    " << prettySize(bytes_written + bytes_read) << " operation in "
            << elapsed_ms << "ms"
            << " (" << prettySize(((bytes_written +
                                    bytes_read) * 1000) / elapsed_ms) << " bytes/s)"
            << std::endl
            << "Elapsed time: " << elapsed_ms << " ms " << std::endl;

  broker.close();

  return 0;
}

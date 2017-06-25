#include "network/BrokerServer.hh"
#include "broker/Broker.hh"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
  int32_t nb_threads;
  int32_t port;
  int32_t broker_id;
  std::string log_dir;

  po::options_description desc("Kafka broker");
  desc.add_options()
    ("help", "produce help message")
    ("nb-threads", po::value<int32_t>(&nb_threads)->default_value(0),
     "Set the number of thread (0 = use the core number)")
    ("port", po::value<int32_t>(&port)->default_value(9000), "Set the port")
    ("broker-id", po::value<int32_t>(&broker_id)->default_value(0), "Set the broker-id")
    ("log-dir", po::value<std::string>(&log_dir)->default_value("/tmp/myKafka"), "Set the log directory")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 1;
  }

  if (log_dir.empty())
  {
    std::cout << "Empty log-dir!" << std::endl;
    return 1;
  }

  Broker::Broker broker(log_dir);
  auto res = broker.load();
  if (res.code() != mykafka::Error::OK)
  {
    std::cout << "Can't load the conf from " << log_dir << ", error is: ["
              << res.code() << "] " << res.msg()
              << std::endl;
    return 1;
  }

  Network::BrokerServer server("0.0.0.0:" + std::to_string(port), broker, nb_threads);
  server.run();

  return 0;
}

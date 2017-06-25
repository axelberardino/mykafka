#include "network/BrokerServer.hh"
#include "broker/Broker.hh"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
  po::options_description desc("Kafka broker");
  desc.add_options()
    ("help", "produce help message")
    ("nb-threads", po::value<int32_t>()->default_value(0), "Set the number of thread (0 = use the core number)")
    ("port", po::value<int32_t>()->default_value(9000), "Set the port")
    ("broker-id", po::value<int32_t>()->default_value(0), "Set the broker-id")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 1;
  }

  Broker::Broker broker;
  Network::BrokerServer server("0.0.0.0:" + std::to_string(vm["port"].as<int>()),
                               broker, vm["nb-threads"].as<int32_t>());
  server.run();

  return 0;
}

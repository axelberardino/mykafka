#include "network/Client.hh"

#include <iostream>
#include <inttypes.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
  po::options_description desc("Kafka producer");
  desc.add_options()
    ("help", "produce help message")
    ("broker-address", po::value<std::string>()->default_value("localhost:9000"), "Set the broker address")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << "Usage: cat file | " << argv[0] << "\n"
              << desc << std::endl;
    return 1;
  }

  const std::string address = vm["broker-address"].as<std::string>();
  Network::Client client(address);
  std::cout << "Start to send to " << address << std::endl;
  std::string line;
  while (std::getline(std::cin, line))
  {
      mykafka::SendMessageRequest request;
      mykafka::SendMessageResponse response;
      request.set_payload(line);
      auto res = client.sendMessage(request, response);
      if (res.ok())
        std::cout << "Payload written at offset " << response.offset() << std::endl;
      else
      {
        std::cout << res.error_code() << ": " << res.error_message() << std::endl;
      }
  }

  return 0;
}

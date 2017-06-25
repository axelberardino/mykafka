#include "network/Server.hh"

int main()
{
  Network::Server server("0.0.0.0:50051");
  server.run();
  return 0;
}

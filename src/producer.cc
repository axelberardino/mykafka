#include "network/Client.hh"

#include <iostream>
#include <inttypes.h>

int main(int argc, char** argv)
{
  Network::Client client("localhost:50051");
  int64_t i = 0;
  while (true)
  {
      mykafka::SendMessageRequest request;
      mykafka::SendMessageResponse response;
      request.set_payload("world " + std::to_string(i));
      auto res = client.sendMessage(request, response);
      if (!res.ok())
        std::cout << res.error_code() << ": " << res.error_message() << std::endl;
      else
        std::cout << "Written at offset " << response.offset() << std::endl;
      ++i;
  }

  return 0;
}

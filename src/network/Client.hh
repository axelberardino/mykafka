#ifndef NETWORK_CLIENT_HH_
# define NETWORK_CLIENT_HH_

# include <grpc++/grpc++.h>
# include <grpc/support/log.h>

# include "mykafka.grpc.pb.h"

# include <memory>

namespace Network
{
  /*!
  ** @class Client
  **
  ** Class use to ease the write of myKafka clients.
  */
  class Client
  {
  public:
    /*!
    ** Initialize a client network.
    **
    ** @param address The server + port (server:port)
    ** @param client_connection_timeout Timeout for a query.
    */
    Client(std::string address, int64_t client_connection_timeout = 200 /* ms */);

    /*!
    ** Force a reconnect. Already called internally, should'nt be called.
    **
    ** @return If reconnection succeed.
    */
    bool reconnect();

    /*!
    ** Send a payload to the broker.
    **
    ** @param request The message containing the payload.
    ** @param response The server's answer.
    ** @param try_reconnect Try to reconnect.
    **
    ** @return grpc::ok on succeed.
    */
    grpc::Status sendMessage(mykafka::SendMessageRequest& request,
                             mykafka::SendMessageResponse& response,
                             bool try_reconnect = false);

    /*!
    ** Get a payload from a given offset.
    **
    ** @param request The message containing the offset.
    ** @param response The server's answer.
    ** @param try_reconnect Try to reconnect.
    **
    ** @return grpc::ok on succeed.
    */
    grpc::Status getMessage(mykafka::GetMessageRequest& request,
                            mykafka::GetMessageResponse& response,
                            bool try_reconnect = false);

    /*!
    ** Create a topic/partition.
    **
    ** @param request The topic/partition.
    ** @param response The server's answer.
    ** @param try_reconnect Try to reconnect.
    **
    ** @return grpc::ok on succeed.
    */
    grpc::Status createPartition(mykafka::TopicPartitionRequest& request,
                                 mykafka::Error& response,
                                 bool try_reconnect = false);

    /*!
    ** Delete a topic/partition.
    **
    ** @param request The topic/partition.
    ** @param response The server's answer.
    ** @param try_reconnect Try to reconnect.
    **
    ** @return grpc::ok on succeed.
    */
    grpc::Status deletePartition(mykafka::TopicPartitionRequest& request,
                                 mykafka::Error& response,
                                 bool try_reconnect = false);

    /*!
    ** Delete a topic.
    **
    ** @param request The topic.
    ** @param response The server's answer.
    ** @param try_reconnect Try to reconnect.
    **
    ** @return grpc::ok on succeed.
    */
    grpc::Status deleteTopic(mykafka::TopicPartitionRequest& request,
                             mykafka::Error& response,
                             bool try_reconnect = false);

    /*!
    ** Get offsets of a topic/partition.
    **
    ** @param request The topic/partition.
    ** @param response The server's answer.
    ** @param try_reconnect Try to reconnect.
    **
    ** @return grpc::ok on succeed.
    */
    grpc::Status getOffsets(mykafka::GetOffsetsRequest& request,
                            mykafka::GetOffsetsResponse& response,
                            bool try_reconnect = false);

    /*!
    ** Get info about a broker.
    **
    ** @param request The topic.
    ** @param response The server's answer.
    ** @param try_reconnect Try to reconnect.
    **
    ** @return grpc::ok on succeed.
    */
    grpc::Status brokerInfo(mykafka::Void& request,
                            mykafka::BrokerInfoResponse& response,
                            bool try_reconnect = false);

  private:
    const std::string address_;
    int64_t client_connection_timeout_;
    int64_t reconnect_timeout_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<mykafka::Broker::Stub> stub_;
  };
} // Network

#endif /* !NETWORK_CLIENT_HH_ */

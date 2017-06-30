cmake_minimum_required(VERSION 3.0)
# Cmake find modules
include(FindBoost)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(MyKafka)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTOS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/protos)
set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(TEST_OUTPUT_PATH /tmp/mykafka-test)
set(PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/${PROTOS_PATH}/mykafka.proto)

add_compile_options(-W -Wall -std=c++11)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 --ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(
  SOURCES

  ${SRC_PATH}/commitlog/Index.cc
  ${SRC_PATH}/commitlog/Segment.cc
  ${SRC_PATH}/commitlog/Partition.cc
  ${SRC_PATH}/utils/Utils.cc
  ${SRC_PATH}/utils/ConfigManager.cc
  ${SRC_PATH}/network/RpcServer.cc
  ${SRC_PATH}/network/BrokerServer.cc
  ${SRC_PATH}/network/Client.cc
  ${SRC_PATH}/network/RpcService.cc
  ${SRC_PATH}/network/GetMessageService.cc
  ${SRC_PATH}/network/SendMessageService.cc
  ${SRC_PATH}/network/GetOffsetsService.cc
  ${SRC_PATH}/network/BrokerInfoService.cc
  ${SRC_PATH}/network/CreatePartitionService.cc
  ${SRC_PATH}/network/DeletePartitionService.cc
  ${SRC_PATH}/network/DeleteTopicService.cc
  ${SRC_PATH}/broker/Broker.cc
  )

include_directories(/usr/local/include)
include_directories(${CMAKE_SOURCE_DIR}/protos)
include_directories(${CMAKE_SOURCE_DIR}/src)
link_directories(/usr/local/lib)

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)
find_package(
  Boost
  1.55.0

  REQUIRED system
  REQUIRED thread
  REQUIRED filesystem
  REQUIRED program_options
  )
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS_PATH} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTOS_PATH} ${PROTOS})

# set(MYKAFKA_PROTO "${PROTO_PATH}/mykafka.proto")
# set(MYKAFKA_PB_CPP_FILE "${PROTOS_PATH}/mykafka.pb.cc")
# set(MYKAFKA_PB_H_FILE "${PROTOS_PATH}/mykafka.pb.h")
# set(MYKAFKA_GRPC_PB_CPP_FILE "${PROTOS_PATH}/mykafka.grpc.pb.cc")
# set(MYKAFKA_GRPC_PB_H_FILE "${PROTOS_PATH}/mykafka.grpc.pb.h")
# add_custom_command(
#   OUTPUT "${MYKAFKA_PB_H_FILE}"
#   "${MYKAFKA_PB_CPP_FILE}"
#   "${MYKAFKA_GRPC_PB_H_FILE}"
#   "${MYKAFKA_GRPC_PB_CPP_FILE}"
#   COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
#   ARGS "--proto_path=${PROTOS_PATH}"
#   "--cpp_out=${PROTOS_PATH}"
#   "${MYKAFKA_PROTO}"
#   COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
#   ARGS "--proto_path=${PROTOS_PATH}"
#   "--grpc_out=${PROTOS_PATH}"
#   "--plugin=protoc-gen-grpc=/usr/local/bin/GRPC_CPP_PLUGIN"
#   "${MYKAFKA_PROTO}"
#   )
# set(GENERATED_PROTOBUF_FILES ${MYKAFKA_PB_H_FILE} ${MYKAFKA_PB_CPP_FILE}
#   ${MYKAFKA_GRPC_PB_H_FILE} ${MYKAFKA_GRPC_PB_CPP_FILE})
# include_directories(${GENERATED_PROTOBUF_PATH})
# link_libraries(grpc++_unsecure grpc gpr ${PROTOBUF_LIBRARY})


Add_executable(
  mykafka-server

  ${PROTO_SRCS}
  ${GRPC_SRCS}
  ${SOURCES}
  ${SRC_PATH}/main-server.cc
  )
target_link_libraries(
  mykafka-server

  ${Boost_LIBRARIES}
  gRPC::grpc++_reflection
  protobuf::libprotobuf
  )
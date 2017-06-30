project(MyKafka)
cmake_minimum_required(VERSION 3.0)

# Cmake find modules
include(FindBoost)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# All path
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(TEST_OUTPUT_PATH /tmp/mykafka-test)
set(PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/protos)
set(PROTO ${PROTO_PATH}/mykafka.proto)

# Binaries names
set(PRODUCER mykafka-producer)
set(CONSUMER mykafka-consumer)
set(SERVER mykafka-server)
set(CONTROL mykafka-ctl)

# Flags
add_compile_options(-W -Wall -std=c++11)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 --ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Sources
set(SOURCES
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
include_directories(${SRC_PATH})

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)
find_package(
  Boost
  1.55.0

  REQUIRED system
  REQUIRED thread
  REQUIRED filesystem
  REQUIRED program_options
  REQUIRED unit_test_framework
  )
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_PATH} ${PROTO})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_PATH} ${PROTO})
include_directories(${PROTO_PATH})

set(ALL_SRC
  ${PROTO_SRCS}
  ${GRPC_SRCS}
  ${SOURCES}
  )
set(ALL_LIB
  ${Boost_LIBRARIES}
  gRPC::grpc++_reflection
  protobuf::libprotobuf
  )

# main binaries
add_executable(${SERVER} ${ALL_SRC} ${SRC_PATH}/main-server.cc)
target_link_libraries(${SERVER} ${ALL_LIB})
add_executable(${PRODUCER} ${ALL_SRC} ${SRC_PATH}/main-producer.cc)
target_link_libraries(${PRODUCER} ${ALL_LIB})
add_executable(${CONSUMER} ${ALL_SRC} ${SRC_PATH}/main-consumer.cc)
target_link_libraries(${CONSUMER} ${ALL_LIB})
add_executable(${CONTROL} ${ALL_SRC} ${SRC_PATH}/main-ctl.cc)
target_link_libraries(${CONTROL} ${ALL_LIB})

# Tests
enable_testing()
set(CMAKE_CTEST_COMMAND ctest -V)

macro(add_unit_test TARGET SRC)
  list(APPEND UNIT_TEST_TARGETS ${TARGET})
  add_executable(${TARGET} EXCLUDE_FROM_ALL ${ALL_SRC} ${SRC})
  set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")
  target_link_libraries(${TARGET} ${ALL_LIB} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
  add_test(${TARGET} ${TEST_PATH}/${TARGET})
endmacro(add_unit_test)

add_unit_test(index-test ${SRC_PATH}/commitlog/Index_Test.cc)
add_unit_test(segment-test ${SRC_PATH}/commitlog/Segment_Test.cc)
add_unit_test(partition-test ${SRC_PATH}/commitlog/Partition_Test.cc)
add_unit_test(config-manager-test ${SRC_PATH}/utils/ConfigManager_Test.cc)
add_unit_test(broker-test ${SRC_PATH}/broker/Broker_Test.cc)


add_custom_target(check DEPENDS ${UNIT_TEST_TARGETS})
add_custom_command(TARGET check
  COMMENT "Run tests"
  POST_BUILD COMMAND ctest ARGS --output-on-failure
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

# Bench
add_executable(commitlog-bench EXCLUDE_FROM_ALL
  ${ALL_SRC} ${SRC_PATH}/commitlog/CommitLog_Bench.cc)
target_link_libraries(commitlog-bench
  ${ALL_LIB} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
set_target_properties(commitlog-bench
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")

add_executable(broker-bench EXCLUDE_FROM_ALL
  ${ALL_SRC} ${SRC_PATH}/broker/Broker_Bench.cc)
target_link_libraries(broker-bench
  ${ALL_LIB})
set_target_properties(broker-bench
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")

add_custom_target(bench DEPENDS commitlog-bench broker-bench)
add_custom_command(TARGET bench
  COMMENT "Run bench"
  POST_BUILD COMMAND
  ${TEST_PATH}/commitlog-bench;
  ${TEST_PATH}/broker-bench
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

# Demo
add_custom_target(demo COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/demo/demo.sh)

# Doc
add_custom_target(doc COMMAND doxygen doc/Doxyfile && cd doc/latex && $(MAKE) && cp refman.pdf ..)

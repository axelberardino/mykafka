CXX = g++
CXXFLAGS += -I/usr/local/include -I./protos/ -I./src/ -pthread -W -Wall -std=c++11
#CXXFLAGS += -O0 -g -ggdb
CXXFLAGS += -O3
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl -lboost_system -lboost_thread \
           -lboost_filesystem -lboost_program_options
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = protos
SRC_PATH = src
TEST_PATH = test
TEST_OUTPUT_PATH = /tmp/mykafka-test

vpath %.proto $(PROTOS_PATH)

PROTOS = $(PROTOS_PATH)/mykafka.proto

SOURCES = \
	$(SRC_PATH)/commitlog/Index.cc \
	$(SRC_PATH)/commitlog/Segment.cc \
	$(SRC_PATH)/commitlog/Partition.cc \
	$(SRC_PATH)/utils/Utils.cc \
	$(SRC_PATH)/utils/ConfigManager.cc \
	$(SRC_PATH)/network/RpcServer.cc \
	$(SRC_PATH)/network/BrokerServer.cc \
	$(SRC_PATH)/network/Client.cc \
	$(SRC_PATH)/network/RpcService.cc \
	$(SRC_PATH)/network/GetMessageService.cc \
	$(SRC_PATH)/network/SendMessageService.cc \
	$(SRC_PATH)/network/GetOffsetsService.cc \
	$(SRC_PATH)/network/BrokerInfoService.cc \
	$(SRC_PATH)/network/CreatePartitionService.cc \
	$(SRC_PATH)/network/DeletePartitionService.cc \
	$(SRC_PATH)/network/DeleteTopicService.cc \
	$(SRC_PATH)/broker/Broker.cc

HEADERS = $(SOURCES:.cc=.hh)

PRODUCER_SRC = $(SRC_PATH)/main-producer.cc
CONSUMER_SRC = $(SRC_PATH)/main-consumer.cc
SERVER_SRC = $(SRC_PATH)/main-server.cc
CONTROL_SRC = $(SRC_PATH)/main-ctl.cc
GRPC_SRC = $(PROTOS:.proto=.grpc.pb.cc)
PB_SRC = $(PROTOS:.proto=.pb.cc)

ALL_SRC = $(GRPC_SRC) $(PB_SRC) $(SOURCES) $(HEADERS) \
          $(PRODUCER_SRC) $(CONSUMER_SRC) $(SERVER_SRC)

OBJ = $(SOURCES:.cc=.o) $(GRPC_SRC:.grpc.pb.cc=.grpc.pb.o) $(PB_SRC:.pb.cc=.pb.o)
PRODUCER_OBJ =  $(OBJ) $(PRODUCER_SRC:.cc=.o)
CONSUMER_OBJ =  $(OBJ) $(CONSUMER_SRC:.cc=.o)
SERVER_OBJ = $(OBJ) $(SERVER_SRC:.cc=.o)
CONTROL_OBJ = $(OBJ) $(CONTROL_SRC:.cc=.o)

PRODUCER = mykafka-producer
CONSUMER = mykafka-consumer
SERVER = mykafka-server
CONTROL = mykafka-ctl

all: $(PRODUCER) $(CONSUMER) $(SERVER) $(CONTROL)

$(PRODUCER): system-check $(PRODUCER_OBJ) $(HEADERS)
	$(CXX) $(PRODUCER_OBJ) $(LDFLAGS) -o $@

$(CONSUMER): system-check $(CONSUMER_OBJ) $(HEADERS)
	$(CXX) $(CONSUMER_OBJ) $(LDFLAGS) -o $@

$(SERVER): system-check $(SERVER_OBJ) $(HEADERS)
	$(CXX) $(SERVER_OBJ) $(LDFLAGS) -o $@

$(CONTROL): system-check $(CONTROL_OBJ) $(HEADERS)
	$(CXX) $(CONTROL_OBJ) $(LDFLAGS) -o $@

Makefile.deps: $(ALL_SRC)
	$(CXX) $(CXXFLAGS) -MM $(ALL_SRC) > Makefile.deps

.PRECIOUS: %.o
%.o: %.cc
	$(CXX) $(CXXFLAGS) $< -c -o $@

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(PROTOS_PATH) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(PROTOS_PATH) $<

$(TEST_PATH)/index-test: $(OBJ) $(SRC_PATH)/commitlog/Index_Test.o
	$(CXX) $(OBJ) $(SRC_PATH)/commitlog/Index_Test.o $(LDFLAGS) -lboost_unit_test_framework -o $@
index-test: check-test $(TEST_PATH)/index-test
	$(TEST_PATH)/$@ --log_level=test_suite

$(TEST_PATH)/segment-test: $(OBJ) $(SRC_PATH)/commitlog/Segment_Test.o
	$(CXX) $(OBJ) $(SRC_PATH)/commitlog/Segment_Test.o $(LDFLAGS) -lboost_unit_test_framework -o $@
segment-test: check-test $(TEST_PATH)/segment-test
	$(TEST_PATH)/$@ --log_level=test_suite

$(TEST_PATH)/partition-test: $(OBJ) $(SRC_PATH)/commitlog/Partition_Test.o
	$(CXX) $(OBJ) $(SRC_PATH)/commitlog/Partition_Test.o $(LDFLAGS) -lboost_unit_test_framework -o $@
partition-test: check-test $(TEST_PATH)/partition-test
	$(TEST_PATH)/$@ --log_level=test_suite

$(TEST_PATH)/config-manager-test: $(OBJ) $(SRC_PATH)/utils/ConfigManager_Test.o
	$(CXX) $(OBJ) $(SRC_PATH)/utils/ConfigManager_Test.o $(LDFLAGS) -lboost_unit_test_framework -o $@
config-manager-test: check-test $(TEST_PATH)/config-manager-test
	$(TEST_PATH)/$@ --log_level=test_suite

$(TEST_PATH)/broker-test: $(OBJ) $(SRC_PATH)/broker/Broker_Test.o
	$(CXX) $(OBJ) $(SRC_PATH)/broker/Broker_Test.o $(LDFLAGS) -lboost_unit_test_framework -o $@
broker-test: check-test $(TEST_PATH)/broker-test
	$(TEST_PATH)/$@ --log_level=test_suite

test: all index-test segment-test partition-test config-manager-test broker-test

$(TEST_PATH)/commitlog-bench: $(OBJ) $(SRC_PATH)/commitlog/CommitLog_Bench.o
	$(CXX) $(OBJ) $(SRC_PATH)/commitlog/CommitLog_Bench.o $(LDFLAGS) -lboost_unit_test_framework -o $@
commitlog-bench: check-test $(TEST_PATH)/commitlog-bench
	$(TEST_PATH)/$@ --log_level=test_suite

$(TEST_PATH)/broker-bench: $(OBJ) $(SRC_PATH)/broker/Broker_Bench.o
	$(CXX) $(OBJ) $(SRC_PATH)/broker/Broker_Bench.o $(LDFLAGS) -o $@
broker-bench: check-test $(TEST_PATH)/broker-bench
	$(TEST_PATH)/$@

bench: commitlog-bench broker-bench

doc/refman.pdf:
	doxygen doc/Doxyfile && cd doc/latex && $(MAKE) && cp refman.pdf ..

doc: doc/refman.pdf

demo:

clean:
	rm -f Makefile.deps $(PROTOS_PATH)/*.cc $(PROTOS_PATH)/*.h
	find . -name "*.o" | xargs rm -f

distclean: clean
	rm -f $(PRODUCER) $(CONSUMER) $(SERVER) ./test/*

PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD = $(PROTOC) --version | grep -q libprotoc.3
PLUGIN_CHECK_CMD = which $(GRPC_CPP_PLUGIN)
TEST_DIR_CHECK_CMD = stat $(TEST_PATH)
TEST_OUTPUT_DIR_CHECK_CMD = stat $(TEST_OUTPUT_PATH)
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
ifeq ($(HAS_PROTOC),true)
HAS_VALID_PROTOC = $(shell $(PROTOC_CHECK_CMD) 2> /dev/null && echo true || echo false)
endif
HAS_PLUGIN = $(shell $(PLUGIN_CHECK_CMD) > /dev/null && echo true || echo false)
HAS_TEST_DIR = $(shell $(TEST_DIR_CHECK_CMD) > /dev/null && echo true || echo false)
HAS_TEST_OUTPUT_DIR = $(shell $(TEST_OUTPUT_DIR_CHECK_CMD) > /dev/null && echo true || echo false)

SYSTEM_OK = false
ifeq ($(HAS_VALID_PROTOC),true)
ifeq ($(HAS_PLUGIN),true)
SYSTEM_OK = true
endif
endif

check-test:
ifneq ($(HAS_TEST_DIR),true)
	mkdir -p $(TEST_PATH)
endif
ifneq ($(HAS_TEST_OUTPUT_DIR),true)
	mkdir -p $(TEST_OUTPUT_PATH)
endif

system-check:
ifneq ($(HAS_VALID_PROTOC),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have protoc 3.0.0 installed in your path."
	@echo "Please install Google protocol buffers 3.0.0 and its compiler."
	@echo "You can find it here:"
	@echo
	@echo "   https://github.com/google/protobuf/releases/tag/v3.0.0"
	@echo
	@echo "Here is what I get when trying to evaluate your version of protoc:"
	@echo
	-$(PROTOC) --version
	@echo
	@echo
endif
ifneq ($(HAS_PLUGIN),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have the grpc c++ protobuf plugin installed in your path."
	@echo "Please install grpc. You can find it here:"
	@echo
	@echo "   https://github.com/grpc/grpc"
	@echo
	@echo "Here is what I get when trying to detect if you have the plugin:"
	@echo
	-which $(GRPC_CPP_PLUGIN)
	@echo
	@echo
endif
ifneq ($(SYSTEM_OK),true)
	@false
endif

#Don't create dependencies when we're cleaning, for instance
NODEPS := clean distclean
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
    -include Makefile.deps
endif

.PHONY: doc demo

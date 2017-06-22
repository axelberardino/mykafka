CXX = g++
CXXFLAGS += -I/usr/local/include -I./protos/ -I./src/ -pthread -W -Wall -std=c++11
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl -lboost_system -lboost_thread
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = protos
SRC_PATH = src
TEST_PATH = test

vpath %.proto $(PROTOS_PATH)

PROTOS = $(PROTOS_PATH)/mykafka.proto
SOURCES = \
	$(SRC_PATH)/commitlog/Index.cc \
	$(SRC_PATH)/commitlog/Utils.cc
HEADERS = $(SOURCES:.cc=.hh)

CLIENT_SRC = $(SRC_PATH)/client.cc
SERVER_SRC = $(SRC_PATH)/server.cc
GRPC_SRC = $(PROTOS:.proto=.grpc.pb.cc)
PB_SRC = $(PROTOS:.proto=.pb.cc)

OBJ = $(SOURCES:.cc=.o) $(GRPC_SRC:.grpc.pb.cc=.grpc.pb.o) $(PB_SRC:.pb.cc=.pb.o)
CLIENT_OBJ = $(CLIENT_SRC:.cc=.o)
SERVER_OBJ = $(SERVER_SRC:.cc=.o)

CLIENT = mykafka-client
SERVER = mykafka-server

all: system-check $(CLIENT) $(SERVER)

$(CLIENT): $(GRPC_SRC) $(PB_SRC) $(OBJ) $(CLIENT_OBJ)
	$(CXX) $(OBJ) $(CLIENT_OBJ) $(LDFLAGS) -o $@

$(SERVER): $(GRPC_SRC) $(PB_SRC) $(OBJ) $(SERVER_OBJ)
	$(CXX) $(OBJ) $(SERVER_OBJ) $(LDFLAGS) -o $@

Makefile.deps: $(GRPC_SRC) $(PB_SRC) $(SOURCES) $(HEADER) $(CLIENT_SRC) $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -MM $(SOURCES) $(CLIENT_SRC) $(SERVER_SRC) > Makefile.deps

.PRECIOUS: %.o
%.o: %.cc
	$(CXX) $(CXXFLAGS) $< -c -o $@

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(PROTOS_PATH) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(PROTOS_PATH) $<

$(TEST_PATH)/index-test: $(SOURCES) $(OBJ) $(SRC_PATH)/commitlog/Index_Test.o
	$(CXX) $(OBJ) $(SRC_PATH)/commitlog/Index_Test.o $(LDFLAGS) -lboost_unit_test_framework -o $@

check-test:
	mkdir -p $(TEST_PATH)

index-test: check-test $(TEST_PATH)/index-test
	$(TEST_PATH)/$@ --log_level=test_suite

test: index-test

clean:
	rm -f Makefile.deps $(PROTOS_PATH)/*.cc $(PROTOS_PATH)/*.h
	find . -name "*.o" | xargs rm -f

distclean: clean
	rm -f $(CLIENT) $(SERVER) ./test/*

PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD = $(PROTOC) --version | grep -q libprotoc.3
PLUGIN_CHECK_CMD = which $(GRPC_CPP_PLUGIN)
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
ifeq ($(HAS_PROTOC),true)
HAS_VALID_PROTOC = $(shell $(PROTOC_CHECK_CMD) 2> /dev/null && echo true || echo false)
endif
HAS_PLUGIN = $(shell $(PLUGIN_CHECK_CMD) > /dev/null && echo true || echo false)

SYSTEM_OK = false
ifeq ($(HAS_VALID_PROTOC),true)
ifeq ($(HAS_PLUGIN),true)
SYSTEM_OK = true
endif
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

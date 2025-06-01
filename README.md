# MyKafka - Kafka Broker C++ Clone

**MyKafka** is a C++ equivalent of a Kafka broker. It supports reading and
writing data. This component handles:
  * Commit log (written by segment/index).
  * Automatic deletion of segments based on partition size or TTL (time to live).
  * Creation and deletion of topics and partitions.
  * Sending and receiving data via RPC.

## DEPENDENCIES

  * A C++11-compatible compiler
  * gRPC 1.3.x
  * Protobuf 3
  * Boost 1.55
  * Doxygen, LaTeX (for documentation generation only)

The install-deps.sh script can assist with installing required components.
(Note: "texlive-full" is very large and only necessary for generating
documentation).

## BUILD

To build the project, simply run:
```bash
make
```

## TESTING

Unit tests can be run using:
```bash
make test
```

Note: Some tests might fail if the number of allowed open files is too low. You
can increase it with:
```bash
sudo ulimit -n 65535
```

## BENCHMARKS

Some benchmarks can be executed with:
```bash
make bench
```

## DEMO

A demo for various scenarios can be run with:
```bash
make demo
```

## DOCUMENTATION

The code is documented using Doxygen. Documentation can be generated with:
```bash
make doc
```

## BINARIES

  * mykafka-broker: The server for receiving data.
  * mykafka-ctl: A tool for managing a broker.
  * mykafka-producer: Sends messages to a broker.
  * mykafka-consumer: Receives messages from a broker.

## API

### SendMessage

Sends a message to a broker.
  * Input
    * topic
    * partition
    * message content
  * Output
    * error code + message
    * offset where the message was written

### GetMessage

Receives a message from a broker.
  * Input
    * topic
    * partition
    * starting offset
  * Output
    * error code + message
    * binary message

### GetOffsets

Gets partition offset info.
  * Input
    * topic
    * partition
  * Output
    * error code + message
    * first offset of the partition
    * last valid of the partition (commit)

Note: the committed offset always equals the latest offset (there's no replication)

### CreatePartition

Creates a partition.
  * Input
    * topic
    * partition
    * [optional] segment size (default 4 KB)
    * [optional] max partition size
    * [optional] segment TTL
  * Output
    * error code + message

### DeletePartition

Deletes an existing partition.
  * Input
    * topic
    * partition
  * Output
    * error code + message

### DeleteTopic

Deletes a whole topic.
  * Input
    * topic
  * Output
    * error code + message

### BrokerInfo

Gets broker information (partitions list, configs, offsets, etc.)
  * Input
    * none
  * Output
    * error code + message
    * pre-formatted text message

# TECHNICAL EXPLANATION


## Commit Log - Segment

A segment consists of two files: an index and a binary log.

The index is made of series of offset + position. An offset is an entry id, et
the position is its physical representation in the binary log. This binary log
is memory-mapped I/O (mmap and pre-allocated to 10 MB), which allows fast
lookups (mostly throught binary search).

The binary log is a classic binary file containing a series of entries with the
format: offset, position, message size, message.

### Example of segment
```text
     001.index                       001.log
 offset, position        offset, position, size, payload
      0,        0             0,        0,    5, "first"
      1,        5             1,        5,    4, "test"
      2,        9             2,        9,   20, "{my_payload:content}"
      3,       29             3,       29,    2, "xx"
````

Files are named using a 20-digit number:
  * <20-digit>.index
  * <20-digit>.log

## Partition

Physically, a partition is a directory containing segments. File names include
their first offset, which allows finding their data based on this given offset.

A partition keeps an "active segment", meaning a pointer on the last available
segment. When a segment is too large, a new one is created. Old segments are
deleted if a max size or TTL was defined at creation.


## Topic

A topic is just a prefix with a partition number.

### Example

  * events-0
  * library-34

To delete a topic, one can simply removes all partitions starting with that prefix.

## Broker

The broker manages a list of partitions, each linked to a binary config file.

A partition is associated to binary configuration. This one is memory mapped and
its size is exactly 32 bytes (4 * int64). This configuration file has: the
segment size, the maximal partition size and the very last valid offset of the
partition.


### Example

```text
Topic:bookstore
        partition 0:
            max_segment_size: 4096
            max_partition_size: 0
            segment_ttl: 0
            first_offset: 500
            next_offset: 456672
            commit_offset: 456652
        partition 1:
            max_segment_size: 1024
            max_partition_size: 4096
            segment_ttl: 2 // sec
            first_offset: 600
            next_offset: 456542
            commit_offset: 456438
Topic:events
        [...] and so on...
```

This broker communicate using gRPC.

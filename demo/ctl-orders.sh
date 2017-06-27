#!/bin/bash

cur_dir=$(basename $(pwd) )
prefix=demo
if [ "$cur_dir" = "demo" ]; then
    prefix=.
fi
source "${prefix}/utils.sh" &>/dev/null

title "KAFKA CONTROLER TOOL"
echo

section "Clean folder $DATA_DIR"
launch "rm -rf $BASE_DIR/*"
launch "mkdir -p $DATA_DIR $LOG_DIR"

section "Launch server"
launch_bg "./$SERVER --log-dir=$DATA_DIR" "$LOG_DIR/server.log"
server_pid=$!

sleep 1

section "Test add topic library, 3 partitions"
launch "./$CTL --topic=library --partition=0 --action=create"
launch "./$CTL --topic=library --partition=1 --action=create"
launch "./$CTL --topic=library --partition=2 --action=create"

section "Test add topic restaurant, 3 partitions"
launch "./$CTL --topic=restaurant --partition=0 --action=create"
launch "./$CTL --topic=restaurant --partition=1 --action=create"
launch "./$CTL --topic=restaurant --partition=2 --action=create"

section "View current state of the server"
launch "./$CTL --action=info"

section "Get current offsets of library/O"
launch "./$CTL --action=offsets --topic=library --partition 0"

section "Launch producer, to insert some words from english dictionnary, please wait..."
launch "head -n 10 /usr/share/dict/british-english | ./$PRODUCER --topic library --partition 0" "$LOG_DIR/producer.log"
text "Inserted 10 words!"
tail -n 10 $LOG_DIR/producer.log

section "Offsets has been change:"
launch "./$CTL --action=offsets --topic=library --partition 0"

section "Let's delete partition 1 of topic library"
launch "./$CTL --action=delete --topic=library --partition 1"

section "Let's delete the entire restaurant topic"
launch "./$CTL --action=delete --topic=restaurant"

section "This is the new server state"
launch "./$CTL --action=info"

section "When creating a partition, we can give it some attributes, like the segment size"
launch "./$CTL --topic=test_option --partition=0 --action=create --segment-size 1024"
section "The max size of a partition (if the physical size is greater, then"
text "the older segments will be deleted until the size fit. The active"
text "segments is never deleted)"
launch "./$CTL --topic=test_option --partition=1 --action=create --partition-size 419430400 #400 Mo"
section "The segment ttl (any old non active segment will be deleted)"
launch "./$CTL --topic=test_option --partition=2 --action=create --segment-ttl 3600 #1 hour"

section "Let's check in server state:"
launch "./$CTL --action=info"

section "Let's check, that the server is able to reload the conf."
launch "kill $server_pid"
launch_bg "./$SERVER --log-dir=$DATA_DIR" "$LOG_DIR/server.log"
server_pid=$!
section "Server state after restart:"
launch "./$CTL --action=info"

section "Stop the server"
launch "kill $server_pid"

wait

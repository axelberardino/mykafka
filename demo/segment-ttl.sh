#!/bin/bash

cur_dir=$(basename $(pwd) )
prefix=demo
if [ "$cur_dir" = "demo" ]; then
    prefix=.
fi
source "${prefix}/utils.sh" &>/dev/null

title "SEGMENT DELETED DUE TO SEGMENT TTL"
echo

section "Clean folder $DATA_DIR"
launch "rm -rf $BASE_DIR/*"
launch "mkdir -p $DATA_DIR $LOG_DIR"

section "Launch server"
launch_bg "./$SERVER --log-dir=$DATA_DIR" "$LOG_DIR/server.log"
server_pid=$!
sleep 1

section "Add topic mytopic, partition 0 segment 1Ko, ttl 2 sec"
launch "./$CTL --topic=mytopic --partition=0 --action=create --segment-size=1024 --segment-ttl=2"

section "Now we add some text"
launch "head -c 768 $EN_DICT | ./$PRODUCER --topic=mytopic --partition 0" "$LOG_DIR/producer.log"

section "Let's check we have 2 files"
text "(there is some overhead because of the header size)"
launch "\ls -lpha $DATA_DIR/mytopic-0/*.log"

section "Now we wait 4 sec"
launch "sleep 4"

section "Let's add few text, to force a new segment (will force a clean)"
launch "head -c 256 $EN_DICT | ./$PRODUCER --topic=mytopic --partition 0" "$LOG_DIR/producer.log"

section "Let's check we have only 1 small file (previous 2 was out of date)"
launch "\ls -lpha $DATA_DIR/mytopic-0/*.log"

section "Stop the server"
launch "kill $server_pid &>/dev/null"

wait

#!/bin/bash

cur_dir=$(basename $(pwd) )
prefix=demo
if [ "$cur_dir" = "demo" ]; then
    prefix=.
fi
source "${prefix}/utils.sh" &>/dev/null

title "SEGMENT DELETED DUE TO PARTITION SIZE"
echo

section "Clean folder $DATA_DIR"
launch "rm -rf $BASE_DIR/*"
launch "mkdir -p $DATA_DIR $LOG_DIR"

section "Launch server"
launch_bg "./$SERVER --log-dir=$DATA_DIR" "$LOG_DIR/server.log"
server_pid=$!

sleep 1

section "Add topic mytopic, partition 0 with a size of 4Ko, segment 1Ko"
launch "./$CTL --topic=mytopic --partition=0 --action=create --segment-size=1024 --partition-size=4096"

section "Now we add some text (8Ko data)"
launch "head -c 8192 /usr/share/dict/british-english | ./$PRODUCER --topic=mytopic --partition 0" "$LOG_DIR/producer.log"

section "Let's check we have only 4 files left"
launch "\ls -lpha $DATA_DIR/mytopic-0/*.log"

section "Stop the server"
launch "kill $server_pid"

wait

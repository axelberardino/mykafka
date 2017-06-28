#!/bin/bash

cur_dir=$(basename $(pwd) )
prefix=demo
if [ "$cur_dir" = "demo" ]; then
    prefix=.
fi
source "${prefix}/utils.sh" &>/dev/null

title "ONE PRODUCER - MANY CONSUMERS"
echo

section "Clean folder $DATA_DIR"
launch "rm -rf $BASE_DIR/*"
launch "mkdir -p $DATA_DIR $LOG_DIR"

section "Launch server"
launch_bg "./$SERVER --log-dir=$DATA_DIR" "$LOG_DIR/server.log"
server_pid=$!
sleep 1

section "Add topic mytopic, partition 0"
launch "./$CTL --topic=mytopic --partition=0 --action=create"

section "We launch 8 consumers, they will consume as soon as there is data on the partition"
consumer_pids=""
for i in $(seq 8); do
    launch_bg "./$CONSUMER --topic=mytopic --partition=0" "$LOG_DIR/consumer-$i.log"
    consumer_pids="$consumer_pids $!"
done

section "Launch the producer, to insert the british dictionnary"
launch "cat $EN_DICT | ./$PRODUCER --topic=mytopic --partition 0" "$LOG_DIR/producer.log"

section "Just wait a little, to let the consumers get the data"
launch "sleep 3"

section "Stop the server, and the consumers"
launch "kill $server_pid $consumer_pids &>/dev/null"

wait

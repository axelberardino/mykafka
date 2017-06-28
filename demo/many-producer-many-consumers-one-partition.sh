#!/bin/bash

cur_dir=$(basename $(pwd) )
prefix=demo
if [ "$cur_dir" = "demo" ]; then
    prefix=.
fi
source "${prefix}/utils.sh" &>/dev/null

title "MANY PRODUCERS - MANY CONSUMERS - 1 PARTITION"
echo

section "Clean folder $DATA_DIR"
launch "rm -rf $BASE_DIR/*"
launch "mkdir -p $DATA_DIR $LOG_DIR"

section "Launch server"
launch_bg "./$SERVER --log-dir=$DATA_DIR" "$LOG_DIR/server.log"
server_pid=$!

sleep 1

section "Add topic mytopic, partition 0 with "
launch "./$CTL --topic=mytopic --partition=0 --action=create"

warn "Launching htop in another terminal could be useful!"
read -p "Press enter to continue this test"

section "We launch 16 consumers, they will consume as soon as there is data on the partition"
text "They will stop after consuming 100k message."
consumer_pids=""
for i in $(seq 16); do
    launch_bg "./$CONSUMER --topic=mytopic --partition=0 --nb-offset 100000" "$LOG_DIR/consumer-$i.log"
    consumer_pids="$consumer_pids $!"
done

section "Launch 8 producers, each will insert the british dictionnary"
producer_pids=""
for i in $(seq 16); do
    launch_bg "cat /usr/share/dict/british-english | ./$PRODUCER --topic=mytopic --partition 0" "$LOG_DIR/producer-$i.log"
    producer_pids="$producer_pids $!"
done

section "Wait for consumer and producer"
launch "wait $consumer_pids $producer_pids"

section "Stop the server, and the consumers"
launch "kill $server_pid $consumer_pids"

wait

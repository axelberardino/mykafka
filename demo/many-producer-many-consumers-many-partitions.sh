#!/bin/bash

cur_dir=$(basename $(pwd) )
prefix=demo
if [ "$cur_dir" = "demo" ]; then
    prefix=.
fi
source "${prefix}/utils.sh" &>/dev/null

title "MANY PRODUCERS - MANY CONSUMERS - MANY PARTITIONS"
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
section "Add topic mytopic, partition 1 with "
launch "./$CTL --topic=mytopic --partition=1 --action=create"
section "Add topic mytopic, partition 2 with "
launch "./$CTL --topic=mytopic --partition=2 --action=create"
section "Add topic mytopic, partition 3 with "
launch "./$CTL --topic=mytopic --partition=3 --action=create"

warn "Launching htop in another terminal could be useful!"
read -p "Press enter to continue this test"

section "We launch 4 consumers on each partition, they will consume as soon as there is data"
text "They will stop after consuming 100k message."
consumer_pids=""

for part in $(seq 4); do
    for i in $(seq 4); do
        launch_bg "./$CONSUMER --topic=mytopic --partition=$part --nb-offset 100000" "$LOG_DIR/consumer-$i.log"
        consumer_pids="$consumer_pids $!"
    done
done

section "Launch 2 producers on each partition, each will insert the british dictionnary"
producer_pids=""

for part in $(seq 4); do
    for i in $(seq 2); do
        launch_bg "cat $EN_DICT | ./$PRODUCER --topic=mytopic --partition $part" "$LOG_DIR/producer-$i.log"
        producer_pids="$producer_pids $!"
    done
done

section "Wait for consumers and producers"
launch "wait $consumer_pids $producer_pids"

section "Stop the server, the producers and the consumers"
launch "kill $server_pid $producer_pids $consumer_pids &>/dev/null"

wait

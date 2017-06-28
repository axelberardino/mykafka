#!/bin/bash

cur_dir=$(basename $(pwd) )
prefix=demo
if [ "$cur_dir" = "demo" ]; then
    prefix=.
fi
source "${prefix}/utils.sh" &>/dev/null

title "SIMPLE PRODUCER-CONSUMER"
echo

section "Clean folder $DATA_DIR"
launch "rm -rf $BASE_DIR/*"
launch "mkdir -p $DATA_DIR $LOG_DIR"

section "Launch server"
launch_bg "./$SERVER --log-dir=$DATA_DIR" "$LOG_DIR/server.log"
server_pid=$!
sleep 1

section "Add topic test_topic, partition 0"
launch "./$CTL --topic=del_topic --partition=0 --action=create"

section "Check that partition exists"
launch "./$CTL --action=info"

section "Launch producer, to insert 100k english words, please wait..."
launch "head -n 100000 $EN_DICT | ./$PRODUCER --topic del_topic --partition 0" "$LOG_DIR/producer.log"
text "Inserted 10k words!"
tail -n 10 $LOG_DIR/producer.log

section "Launch a consumer in background"
launch_bg "./$CONSUMER --topic=del_topic --partition=0 --stop-if-no-message=true" "$LOG_DIR/consumer.log"
consumer_pid=$!

section "Wait a little to let the consumer get some messages"
sleep 1

section "Delete the topic on which the consumer works"
launch "./$CTL --topic=del_topic --partition=0 --action=delete"

section "Consumer reaction"
tail -n 5 $LOG_DIR/consumer.log

section "Stop the server"
launch "kill $server_pid $consumer_pid &>/dev/null"

wait

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

section "Add topic test_topic, partition 1"
launch "./$CTL --topic=test_topic --partition=1 --action=create"

section "Check that partition exists"
launch "./$CTL --action=info"

section "Launch producer, to insert english dictionnary, please wait..."
launch "cat /usr/share/dict/british-english | ./$PRODUCER --topic test_topic --partition 1" "$LOG_DIR/producer.log"
text "Inserted $(cat /usr/share/dict/british-english | wc -l) words!"
tail -n 10 $LOG_DIR/producer.log

section "Launch consumer, to get the words, please wait..."
launch "./$CONSUMER --topic=test_topic --partition=1 --stop-if-no-message=true" "$LOG_DIR/consumer.log"
text "Execution finished"
tail -n 10 $LOG_DIR/consumer.log

section "Stop the server"
launch "kill $server_pid"

wait

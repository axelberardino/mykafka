#!/bin/bash

finish()
{
    trap - SIGTERM # Disable sigterm trap to avoid signal recursion
    echo "Sigterm detected, cleaning process..."
    kill 0
}
trap "exit 1" SIGTERM
trap finish 0 1 2 3 13 # EXIT HUP INT QUIT PIPE

for i in $(seq 10); do
    cat /usr/share/dict/british-english | ./mykafka-producer --topic toto --partition 1 &
done

for i in $(seq 5); do
    ./mykafka-consumer --topic toto --partition 1 &
done


wait

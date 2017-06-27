#!/bin/bash

BASE_DIR=/tmp/mykafka-demo
DEMOS="simple-producer-consumer.sh ctl-orders.sh one-producer-many-consumers.sh"
#DEMOS="one-producer-many-consumers.sh"

trap "echo" SIGTERM

echo -e "\033[31;1m"
echo "  _____  ______ __  __  ____  "
echo " |  __ \|  ____|  \/  |/ __ \ "
echo " | |  | | |__  | \  / | |  | |"
echo " | |  | |  __| | |\/| | |  | |"
echo " | |__| | |____| |  | | |__| |"
echo " |_____/|______|_|  |_|\____/ "
echo "                              "
echo -e "\033[0m"

cur_dir=$(basename $(pwd) )
prefix=demo
if [ "$cur_dir" = "demo" ]; then
    prefix=.
fi

for demo in $DEMOS; do
    ./${prefix}/${demo} "$BASE_DIR"
    read -p "Press enter to start the next demo"
done

#TODO
#
# Test delete partition, while read.
# Test delete topic, while read.

# => demander press -p + htop en "warn"
# Test gros bench 1, 1 partition, 8 writer, 16 lecteurs infinis.
# Test gros bench 2, 8 partitions, * (1 writer, 2 lecteurs infinis).

# 1 writer: max partition size 40 Mo, segsize 4 Mo
# 1 writer: ttl 1 sec, segsize 1024 * 1024 (1 Mo). Constater qu'il ne reste qu'un seul log

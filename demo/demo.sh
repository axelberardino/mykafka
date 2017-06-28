#!/bin/bash

BASE_DIR=/tmp/mykafka-demo
DEMOS="simple-producer-consumer.sh ctl-orders.sh \
 one-producer-many-consumers.sh partition-size.sh \
 segment-ttl.sh many-producer-many-consumers-one-partition.sh \
 many-producer-many-consumers-many-partitions.sh"
DEMOS="many-producer-many-consumers-many-partitions.sh"

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
# Bien "tuner" le segment size !
# Test gros bench 1, 1 partition, 8 writer, 16 lecteurs infinis.
# Test gros bench 2, 8 partitions, * (1 writer, 2 lecteurs infinis).

#!/bin/bash

BASE_DIR=/tmp/mykafka-demo
DEMOS="simple-producer-consumer.sh one-producer-many-consumers.sh"

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

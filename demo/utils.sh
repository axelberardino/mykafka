#!/bin/bash

finish()
{
    trap - SIGTERM # Disable sigterm trap to avoid signal recursion
    kill 0
}
trap "exit 0" SIGTERM
trap finish 0 1 2 3 13 # EXIT HUP INT QUIT PIPE

title()
{
    echo -e "\n\033[32;4m-=-=-=-=-=-=-=- ${1} -=-=-=-=-=-=-=-\033[0m\n"
}

section()
{
    echo -e "\n\033[34m--> ${1}\033[0m"
}

text()
{
    echo -e "\033[34m${1}\033[0m"
}

warn()
{
    echo -e "\033[31m/!\\ ${1} /!\\ \033[0m"
}

abort()
{
    echo "$1"
    exit 1
}

launch()
{
    echo -ne "\033[33m"
    echo -n $1
    echo -e "\033[0m"
    if [ $# -eq 1 ]; then
        eval $1
    else
        echo -e "\033[33mLog can be viewed here: $2\033[0m"
        eval $1 &> "$2"
    fi
}

launch_bg()
{
    echo -ne "\033[33m"
    echo -n $1
    echo -e "\033[0m"
    if [ $# -eq 1 ]; then
        eval $1 &
    else
        echo -e "\033[33mLog can be viewed here: $2\033[0m"
        eval $1 &> "$2" &
    fi
}

SERVER=mykafka-server
stat $SERVER &>/dev/null || SERVER=../$SERVER
stat $SERVER &>/dev/null || abort "Can't find myKafka server"

PRODUCER=mykafka-producer
stat $PRODUCER &>/dev/null || PRODUCER=../$PRODUCER
stat $PRODUCER &>/dev/null || abort "Can't find myKafka producer"

CONSUMER=mykafka-consumer
stat $CONSUMER &>/dev/null || CONSUMER=../$CONSUMER
stat $CONSUMER &>/dev/null || abort "Can't find myKafka consumer"

CTL=mykafka-ctl
stat $CTL &>/dev/null || CTL=../$CTL
stat $CTL &>/dev/null || abort "Can't find myKafka ctl"

BASE_DIR="${1:-/tmp/mykafka-demo}"
DATA_DIR="$BASE_DIR/data"
LOG_DIR="$BASE_DIR/log"

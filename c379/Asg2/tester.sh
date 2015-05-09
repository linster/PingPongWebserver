#! /bin/bash

#Created by EMIL WYROD on the discussion board

PORT=$1
TOTAL=$2
SLEEP_TIME=$3
EXPR=$4
for (( x=0; x<$TOTAL; x++ )); do
    echo -e "$EXPR" | nc localhost "$PORT" &
    sleep $SLEEP_TIME
done

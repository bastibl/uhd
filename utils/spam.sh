#!/bin/bash

APP=../host/build/examples/cs_tx_from_file
FILE=./wifi_frame.bin
DELAY=0
FREQ=5890000000
RXGAIN=10
TXGAIN=35
RATE=10000000

${APP} --file=${FILE} --delay=${DELAY} --freq=${FREQ} --rate=${RATE} --txgain=${TXGAIN} --use-cs=true --cs-hw-threshold=5000 --slot-time=1300 --rxgain=${RXGAIN} --args="addr=192.168.10.3"

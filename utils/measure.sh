#!/bin/bash

OUTFILE="/tmp/csma.csv"

### UHD APP
APP=../host/build/examples/cs_tx_from_file
FILE=./wifi_frame.bin
DELAY=0
FREQ=5890000000
RXGAIN=30
TXGAIN=0
RATE=10000000


### delete?
if [ -e ${OUTFILE} ]
then
	rm -i ${OUTFILE}
fi
### not deleted? then don't touch
if [ -e ${OUTFILE} ]
then
	exit
fi

ssh alix1 "src/gr-ieee802-11/utils/monitor.sh 178 wlan0"
#ssh alix2 "src/gr-ieee802-11/utils/monitor.sh 178 wlan0"
ssh alix2 "sudo iwconfig wlan0 rate 6M"


ssh alix1 "sudo tshark -i wlan0 -T fields -e frame.time_relative -e frame.len -e wlan.da" >  ${OUTFILE} &
sleep 2

${APP} --file=${FILE} --delay=${DELAY} --freq=${FREQ} --rate=${RATE} --txgain=${TXGAIN} --use-cs=true --cs-hw-threshold=90 --cs-num-samps=5 --slot-time=1800 --rxgain=${RXGAIN} &

#ssh alix2 "sudo /home/bloessl/src/gr-ieee802-11/utils/packetspammer/basti_spammer wlan0 -r 3000 -n 9999999 -s 42" &
ssh alix2 "sudo /home/bloessl/src/gr-ieee802-11/utils/packetspammer/udp.py" &


### block here
echo "enter to cancel"
read

###
echo "shutting down"
ssh alix1 "sudo killall tshark"
ssh alix1 "sudo killall -KILL tshark"

ssh alix2 "sudo killall basti_spammer"
ssh alix2 "sudo killall python"
killall `basename ${APP}`

#!/bin/bash

APP=~/usr/lib/uhd/utils/usrp_n2xx_net_burner.py
ADDR="192.168.10.4"
FPGA_DIR="/home/basti/src/uhd/fpga/usrp2/top/N2x0/build-N210R4"
FW_DIR="/home/basti/src/uhd/firmware/zpu/build/usrp2p"


# rename FPGA image
cp ${FPGA_DIR}/u2plus.bin ${FPGA_DIR}/n210_r4_fpga.bin

# burn images
${APP} --addr=${ADDR} --fpga=${FPGA_DIR}/n210_r4_fpga.bin --fw=${FW_DIR}/usrp2p_txrx_uhd.bin --reset

# burn firmware only
#${APP} --addr=${ADDR} --fw=${FW_DIR}/usrp2p_txrx_uhd.bin --reset

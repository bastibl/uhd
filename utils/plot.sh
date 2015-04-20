#!/bin/bash

od -v -Ad -f -w4 /tmp/power.bin | gnuplot -p -e "plot '-' using 1:2"

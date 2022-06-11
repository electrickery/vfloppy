#!/bin/sh
#
TTY='/dev/ttyUSB0'
IMG0='./images/arcs.d88'
IMG1='./images/px8_eps-tf20.d88'
IMG2='./images/px8games.d88'
IMG3='./images/extrautil.d88'
LOG=2

echo ./epspdv3 -d $LOG -s $TTY -0 $IMG0 -1 $IMG1 -2 $IMG2 -3 $IMG3
./epspdv3 -d $LOG -s $TTY -0 $IMG0 -1 $IMG1 -2 $IMG2 -3 $IMG3

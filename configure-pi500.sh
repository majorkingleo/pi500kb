#!/usr/bin/env bash

if ! test -d build ; then
	mkdir build
fi

cd build && cmake .. -DKEYBOARD_VID=0x2e8a -DKEYBOARD_PID=0x0010 -DKEYBOARD_DEV=/dev/input/by-id/usb-Raspberry_Pi_Ltd_Pi_500_Keyboard-event-kbd

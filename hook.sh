#!/bin/bash
# Basic example pi400kb hook
# This script is given the argument 1 for grabbed, 0 for ungrabbed.
echo none > /sys/class/leds/PWR/trigger
case $1 in
    0) # Ungrabbed
        echo 0 > /sys/class/leds/PWR/brightness
        ;;
    1) # Grabbed
        echo 1 > /sys/class/leds/PWR/brightness
        ;;
esac

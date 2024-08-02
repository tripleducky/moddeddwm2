#!/bin/sh

feh --no-fehbg --bg-scale '/home/andre/Pictures/sunset_312-wallpaper-2560x1440.jpg' &

udevadm info /dev/input/by-id/*-mouse || check=0

if [[ $check == 0  ]]
then
    /usr/bin/xinput enable 'DELL0A6E:00 04F3:317E Touchpad'
    echo 'Touchpad enabled. No external mouse detected.'
else
    /usr/bin/xinput disable 'DELL0A6E:00 04F3:317E Touchpad'
    echo 'Touchpad disabled. External mouse detected.'
fi

#!/bin/sh
while (true); do
	RESOLUTION=$(DISPLAY=:0 xrandr | sed -n '/HDMI1 connected/p')
        echo $RESOLUTION | grep HDMI1
        if [ -n "$RESOLUTION" ]; then
                DISPLAY=:0 xrandr --output HDMI1 --auto
        fi
        sleep 5
done

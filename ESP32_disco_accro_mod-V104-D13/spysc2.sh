#!/bin/sh

ip="192.168.42.25"
set="/dev/input/event0"
IFS=$' \n'

while true; do
	evtest ${set} | while read -r f1 f2 f3 f4 f5 f6 f7 cod f9 f10 val; do
        if [ $f7="code" ]; then
            case $cod in
            0|1|2|3|292|293|294) echo -e "g$cod $val" | ./data/lib/ftp/uavpal/bin/netcat-arm -u ${ip} 8888 -w1&;;
            esac
        fi 	
	done
done

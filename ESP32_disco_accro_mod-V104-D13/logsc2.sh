#/bin/sh
/data/ftp/uavpal/bin/adb start-server
ip_sc2=`netstat -nu | grep 9988 | head -1 | awk '{ print $5 }' | cut -d ':' -f 1`
/data/ftp/uavpal/bin/adb connect ${ip_sc2}:9050
/data/ftp/uavpal/bin/adb shell
/data/ftp/uavpal/bin/adb kill-server

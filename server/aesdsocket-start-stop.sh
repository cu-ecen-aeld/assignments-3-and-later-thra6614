#!/bin/sh

#/*******************************************************************************
#* aesdsocket-start-stop.sh
#* Referenced from:      Raghu Sai Phani Sriraj Vemparala, raghu.vemparala@colorado.edu
#******************************************************************************/


if [ "$1" = "start" ]; then
    echo "Starting  Daemon"
    start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
elif [ "$1" = "stop" ]; then
    echo "Stopping  Daemon"
    start-stop-daemon -K -n aesdsocket
else
    echo "Usage: $0 {start|stop}"
    exit 1
fi

exit 0

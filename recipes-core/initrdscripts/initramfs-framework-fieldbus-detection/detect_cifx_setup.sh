#!/bin/sh

if [ "$1" = "init" ]; then
    modprobe uio_netx
else
    modprobe -r uio_netx
fi

#/usr/bin/env bash

#dst_ip=$1
dst_ip=192.168.1.75

scp -r /bridge/src     root@$dst_ip:/bridge
scp -r /bridge/scripts root@$dst_ip:/bridge

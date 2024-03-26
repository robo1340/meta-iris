#/usr/bin/env bash

host_ip=$1
radio_ip=$2
mtu_size=$3
host_ser=$4
radio_ser=$5

echo "SCRIPT: creating virtual serial interfaces"
#create the virtual interfaces
socat -d pty,rawer,echo=0,link=$host_ser,b115200 pty,rawer,echo=0,link=$radio_ser,b115200 &
sleep 0.2

echo "SCRIPT: calling realpath"
p=$(realpath $host_ser)
echo $p
echo "SCRIPT: changing permissions"
chmod 666 $p
chmod 666 $(realpath $radio_ser)

sleep 0.2
echo "SCRIPT: calling slattach"
slattach -d -s 115200 -p slip $p &
sleep 0.5
ifconfig sl0 $radio_ip pointopoint $host_ip mtu $mtu_size up

#/bin/sh

host_ip=$1
radio_ip=$2
mtu_size=$3

killall socat
killall slattach
sleep 1

echo "creating virtual serial interfaces"
#create the virtual interfaces
socat -d pty,rawer,echo=0,link=/dev/ttyV0,b115200 pty,rawer,echo=0,link=/dev/ttyV1,b115200 &
sleep 1

echo "calling realpath"
p=$(realpath /dev/ttyV0)

echo "changing permissions"
chmod 666 $p
chmod 666 $(realpath /dev/ttyV1)

sleep 1
echo "calling slattach"
slattach -s 115200 -p slip $p &
ifconfig sl0 $radio_ip pointopoint $host_ip mtu $mtu_size up
#ifconfig sl0 $radio_ip pointopoint $host_ip mtu 100 up
sleep 1

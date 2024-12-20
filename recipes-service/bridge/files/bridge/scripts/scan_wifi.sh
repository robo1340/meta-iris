systemctl stop wifi
ip link set wlu1 up
iw wlu1 scan | grep SSID:

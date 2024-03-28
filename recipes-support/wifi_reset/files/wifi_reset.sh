#/bin/sh -c "echo 0 > /sys/bus/usb/devices/2-1/authorized"
#/bin/sh -c "echo 1 > /sys/bus/usb/devices/2-1/authorized"
echo 0 > /sys/bus/usb/devices/2-1/authorized
echo 1 > /sys/bus/usb/devices/2-1/authorized

###help
#script to search for device paths by product and vendor id in case path changes
#for X in /sys/bus/usb/devices/*; do 
#    echo "$X"
#    cat "$X/idVendor" 2>/dev/null 
#    cat "$X/idProduct" 2>/dev/null
#    echo
#done

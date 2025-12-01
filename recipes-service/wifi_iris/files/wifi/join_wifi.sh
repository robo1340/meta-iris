killall hostapd
killall wpa_supplicant
ip addr flush dev wlu1
rm /tmp/wpa_supplicant.conf 

if [ "$#" -lt 1 ]; then
	echo "Must specify SSID to join"
	exit 1
fi

ssid=$1

if [ "$#" -ne 2 ]; then
	echo "No Passphrase passed in"
  echo -e "network={\nssid=\"${ssid}\"\n key_mgmt=NONE\n}" > /tmp/wpa_supplicant.conf 
	wpa_supplicant -B -i wlu1 -c /tmp/wpa_supplicant.conf
else
  passphrase=$2
  wpa_passphrase $ssid $passphrase > /tmp/wpa_supplicant.conf
  wpa_supplicant -B -i wlu1 -c /tmp/wpa_supplicant.conf
fi

udhcpc -q -i wlu1
#sleep 2
ip addr show dev wlu1

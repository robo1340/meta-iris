
print_help() {
  echo "Welcome Iris User!"
  echo "In order to help you this short guide has been prepared for you with easy to use commands that acomplish the most common development tasks"
  echo "You can always print off this guide again by typing \"help\""
  echo ""
  echo "bl            | view the bridge log" 
	echo "send          | broadcast a short chat message to other radios"
  echo "bridge-config | reconfigure the radio bridge"
  echo "scan          | scan for available wifi SSIDs"
  echo "join          | join a wifi network first arg is SSID, second is passphrase"
  echo "ap            | bring up the wifi access point"
  echo "sys           | simple alias for systemctl"
}

alias sys='sys_alias() { systemctl "$1" "$2"; }; sys_alias'
alias bl='tail -f /var/log/bridge.log'
alias send='/bridge/src/prj-zmq-send/test'
alias bridge-config='python3 /bridge/scripts/bridge_configure.py'
alias scan='/bridge/scripts/scan_wifi.sh'
#alias join='join() { /bridge/scripts/join_wifi.sh "$1" "$2"; }; join'
alias join='/bridge/scripts/join_wifi.sh'
alias ap='systemctl restart wifi'
alias help="print_help"


print_help

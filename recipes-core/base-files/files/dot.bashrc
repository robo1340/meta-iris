
print_help() {
  echo "Welcome Iris User!"
  echo "In order to help you this short guide has been prepared for you with easy to use commands that acomplish the most common development tasks"
  echo "You can always print off this guide again by typing \"help\""
  echo ""
  echo "bl            | view the bridge log" 
  echo "sl            | view the snap log" 
  echo "hl            | view the hub log" 
  echo "nl            | view the node log" 
  echo "tailall       | view the snap,node,hub,and wifi logs" 
	echo "send          | broadcast a short chat message to other radios"
  echo "bridge-config | reconfigure the radio bridge"
  echo "snap-config   | reconfigure the snap service"
  echo "scan          | scan for available wifi SSIDs"
  echo "join          | join a wifi network first arg is SSID, second is passphrase"
  echo "ap            | bring up the wifi access point"
  echo "sys           | simple alias for systemctl"
  echo "tailf         | follow a log file"
  echo "ussh          | ssh without strict host key checking"
  echo "uscp          | scp without scrit host key checking"
  echo "key_pub       | start the key publishing client"
  echo "servo         | take direct control of several servos"
  echo "version       | get software version"
}

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias sys='sys_alias() { systemctl "$1" "$2"; }; sys_alias'
alias tailf='tailf_alias() { tail -F "$1"; }; tailf_alias'
alias sl='tail -F /var/log/snap.log'
alias hl='tail -F /var/log/hub.log'
alias nl='tail -F /var/log/node.log'
alias bl='tail -F /var/log/bridge.log'
alias tailall='tail -F /var/log/snap.log /var/log/hub.log /var/log/node.log /var/log/wifi.log'
alias send='/bridge/src/prj-zmq-send/test'
alias bridge-config='python3 /bridge/scripts/bridge_configure.py'
alias snap-config='python3 /snap/scripts/snap_configure.py'
alias scan='/home/wifi/scan_wifi.sh'
#alias join='join() { /bridge/scripts/join_wifi.sh "$1" "$2"; }; join'
alias join='/home/wifi/join_wifi.sh'
alias ap='systemctl restart wifi'
alias key_pub='python3 /key_pub/src/main.py'
alias servo='python3 /rover/scripts/servo.py'
alias help="print_help"
alias version='cat /home/version.json'


print_help

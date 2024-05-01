
print_help() {
  echo "Welcome Iris User!"
  echo "In order to help you this short guide has been prepared for you with easy to use commands that acomplish the most common development tasks"
  echo "You can always print off this guide again by typing \"help\""
  echo ""
  echo "send          | broadcast a short chat message to other radios"
  echo "bridge-config | reconfigure the radio bridge"
  echo "sys           | simple alias for systemctl"
}

alias sys='sys_alias() { systemctl "$1" "$2"; }; sys_alias'
#alias send='send_chat() { /bridge/src/prj-zmq-send/test "$1" "$2";}; send_chat'
alias send='/bridge/src/prj-zmq-send/test'
alias bridge-config='python3 /bridge/scripts/bridge_configure.py'
alias help="print_help"

print_help

dstip=$1

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'

uscp -r /hub/src root@$dstip:/hub
ussh root@$dstip 'systemctl restart hub'
ussh root@$dstip 'sync'


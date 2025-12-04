dstip=$1

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'

uscp -r /rover root@$dstip:/
ussh root@$dstip 'systemctl restart rover'
ussh root@$dstip 'sync'


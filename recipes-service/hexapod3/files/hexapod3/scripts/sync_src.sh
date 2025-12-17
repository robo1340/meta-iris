dstip=$1

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'

uscp -r /hexapod root@$dstip:/
ussh root@$dstip 'systemctl restart hexapod'
ussh root@$dstip 'sync'


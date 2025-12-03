dstip=$1

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'

uscp -r /key_pub root@$dstip:/
ussh root@$dstip 'sync'


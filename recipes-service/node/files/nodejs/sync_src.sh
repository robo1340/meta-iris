dstip=$1

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'

uscp /home/nodejs/*.js root@$dstip:/home/nodejs/
uscp /home/nodejs/*.html root@$dstip:/home/nodejs/
uscp /home/nodejs/*.css root@$dstip:/home/nodejs/

ussh root@$dstip 'systemctl restart node'
ussh root@$dstip 'sync'


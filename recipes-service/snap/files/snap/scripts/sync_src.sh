dstip=$1

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'

cd /snap
tar  czf /tmp/src.tgz ./src
cd -
uscp /tmp/src.tgz root@$dstip:/tmp/
ussh root@$dstip 'systemctl stop snap'
ussh root@$dstip 'rm -rf /snap/src'
ussh root@$dstip 'tar -xzf /tmp/src.tgz -C /snap'
ussh root@$dstip 'sync'
ussh root@$dstip 'systemctl start snap'
ussh root@$dstip 'systemctl restart hub'


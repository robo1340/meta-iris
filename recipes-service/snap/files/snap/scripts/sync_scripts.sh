dstip=$1

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'

cd /snap
tar  czf /tmp/scripts.tgz ./scripts
cd -
uscp /tmp/scripts.tgz root@$dstip:/tmp/
ussh root@$dstip 'rm -rf /snap/scripts'
ussh root@$dstip 'tar -xzf /tmp/scripts.tgz -C /snap'
ussh root@$dstip 'sync'


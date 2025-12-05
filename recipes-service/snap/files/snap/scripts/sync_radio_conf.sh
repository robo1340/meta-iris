dstip=$1

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'

cd /snap/conf
tar  czf /tmp/radio.tgz ./si4463
cd -
uscp /snap/conf/config.ini root@$dstip:/snap/conf/
uscp /tmp/radio.tgz root@$dstip:/tmp/
ussh root@$dstip 'rm -rf /snap/conf/si4463'
ussh root@$dstip 'tar -xzf /tmp/radio.tgz -C /snap/conf/'
ussh root@$dstip 'sync'
ussh root@$dstip 'systemctl start snap'
ussh root@$dstip 'systemctl restart hub'


dstip=$1

cd /bridge
tar  czf /tmp/src.tgz ./src
cd -
scp /tmp/src.tgz root@$dstip:/tmp/
ssh root@$dstip 'systemctl stop bridge'
ssh root@$dstip 'rm -rf /bridge/src'
ssh root@$dstip 'tar -xzf /tmp/src.tgz -C /bridge'
ssh root@$dstip 'sync'
ssh root@$dstip 'systemctl start bridge'


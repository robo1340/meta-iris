dstip=$1

cd /bridge
tar  czf /tmp/src.tgz ./src
cd -
scp /tmp/src.tgz root@$dstip:/tmp/
ssh -y root@$dstip 'systemctl stop bridge'
ssh -y root@$dstip 'rm -rf /bridge/src'
ssh -y root@$dstip 'tar -xzf /tmp/src.tgz -C /bridge'
ssh -y root@$dstip 'sync'
ssh -y root@$dstip 'systemctl start bridge'


dstip=$1

cd /bridge
tar  czf /tmp/scripts.tgz ./scripts
cd -
scp /tmp/scripts.tgz root@$dstip:/tmp/
ssh -y root@$dstip 'rm -rf /bridge/scripts'
ssh -y root@$dstip 'tar -xzf /tmp/scripts.tgz -C /bridge'
ssh -y root@$dstip 'sync'


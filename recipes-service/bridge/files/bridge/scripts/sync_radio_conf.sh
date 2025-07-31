dstip=$1

cd /bridge/conf
tar  czf /tmp/radio.tgz ./si4463
cd -
scp /tmp/radio.tgz root@$dstip:/tmp/
ssh -y root@$dstip 'rm -rf /bridge/conf/si4463'
ssh -y root@$dstip 'tar -xzf /tmp/radio.tgz -C /bridge/conf/'
ssh -y root@$dstip 'sync'


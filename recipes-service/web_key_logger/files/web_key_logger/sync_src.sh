dstip=$1

alias ussh='ssh -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
alias uscp='scp -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'

uscp /home/web_key_logger/*.js root@$dstip:/home/web_key_logger/
uscp /home/web_key_logger/*.html root@$dstip:/home/web_key_logger/
uscp /home/web_key_logger/*.css root@$dstip:/home/web_key_logger/

ussh root@$dstip 'systemctl restart web_key_logger'
ussh root@$dstip 'sync'


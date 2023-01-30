ip=`hostname -I | cut -d " " -f 1`
./lcd $ip
echo LCD: $ip

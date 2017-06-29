sudo -u root sshpass -p 1234 ssh 192.168.122.190 "cd /home/vm1/experiments/; bash stop.sh" 
#sudo -u root sshpass -p 1234 ssh 192.168.122.190 "poweroff"

#sleep 10

#rmmod cgtmem.ko

echo "STOPPER: Closed container and cleared cache"

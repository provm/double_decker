# Clear Kernel Log
bash /home/prashanth/Workspace/mem_reclamation/experiments/clear_logs.sh

# Insert kernel module to enable tracking
sudo insmod ../mtp/mtp.ko

gnome-terminal -x bash -c 'sudo -u prashanth sshpass -p "hondacivic" ssh prashanth@10.129.26.195 "cd testing/ycsb; bin/ycsb.sh run mongodb -s -P workloads/workload1 -p mongodb.url=mongodb://10.129.28.6:2001/ycsb?w=1 -p maxexecutiontime=300  -threads 2
-target 3800" > plotter/m1.txt 2>&1 ;'

gnome-terminal -x bash -c 'sudo -u prashanth sshpass -p "hondacivic" ssh prashanth@10.129.26.195 "cd testing/ycsb; bin/ycsb.sh run mongodb -s -P workloads/workload2 -p mongodb.url=mongodb://10.129.28.6:2002/ycsb?w=1 -p maxexecutiontime=300 -threads 2
-target 3800" > plotter/m2.txt 2>&1 ;'

gnome-terminal -x bash -c 'sudo -u prashanth sshpass -p "hondacivic" ssh prashanth@10.129.26.195 "cd testing/ycsb; bin/ycsb.sh run redis -s -P workloads/workload1 -p redis.host=10.129.28.6 -p redis.port=3001 -p maxexecutiontime=300 -threads 2 -target 3800" > plotter/r1.txt 2>&1 ;'

gnome-terminal -x bash -c 'sudo -u prashanth sshpass -p "hondacivic" ssh prashanth@10.129.26.195 "cd testing/ycsb; bin/ycsb.sh run redis -s -P workloads/workload2 -p redis.host=10.129.28.6 -p redis.port=3002 -p maxexecutiontime=300 -threads 2 -target 3800" > plotter/r2.txt 2>&1 ;'

# Start Collected
python /home/prashanth/Workspace/mem_reclamation/experiments/plotter/collector.py &
echo "Collector PID: "$!

# 40s to let processes from containers to settle in
sleep 50

# Bring in externel processes with pressure one after the other after 20s intevels
for var in 18500 17000 15500 14000 12500 11000 9500 8000 6500 5000 3500  
do
        echo $var
        gnome-terminal -x bash -c "sudo -u prashanth sshpass -p 'hondacivic' ssh synerg@10.129.34.7 virsh setmem MTP-1 "$var"M;"
        sleep 20
done

gnome-terminal -x bash -c "sudo -u prashanth sshpass -p 'hondacivic' ssh synerg@10.129.34.7 virsh setmem MTP-1 20000M;"

# 20 seconds for reclamation to reach normal
sleep 30

docker stop m1 m2 r1 r2 

# Remove Kernel Module
sudo rmmod mtp.ko

# Copy files from remote to local
#sshpass -p "hondacivic" scp -r prashanth@10.129.26.185:plotter .

# Output syslog to desired location
cat /var/log/syslog > /home/prashanth/Workspace/mem_reclamation/experiments/plotter/log


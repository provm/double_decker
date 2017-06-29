# Clear Kernel Log
bash /home/prashanth/Workspace/mem_reclamation/experiments/clear_logs.sh

docker start m1 m2 r1 r2
#sleep 30

# Insert kernel module to enable tracking
sudo insmod ../module/kernel_project.ko

gnome-terminal -x bash -c 'sudo -u prashanth sshpass -p "hondacivic" ssh prashanth@10.129.26.195 "cd testing/ycsb; bin/ycsb.sh run mongodb -s -P workloads/workload1 -p mongodb.url=mongodb://10.129.28.6:2001/ycsb?w=1 -p maxexecutiontime=440  -threads 2" > plotter/m1.txt 2>&1 ;'

gnome-terminal -x bash -c 'sudo -u prashanth sshpass -p "hondacivic" ssh prashanth@10.129.26.195 "cd testing/ycsb; bin/ycsb.sh run mongodb -s -P workloads/workload2 -p mongodb.url=mongodb://10.129.28.6:2002/ycsb?w=1 -p maxexecutiontime=440 -threads 2" > plotter/m2.txt 2>&1 ;'

gnome-terminal -x bash -c 'sudo -u prashanth sshpass -p "hondacivic" ssh prashanth@10.129.26.195 "cd testing/ycsb; bin/ycsb.sh run redis -s -P workloads/workload1 -p redis.host=10.129.28.6 -p redis.port=3001 -p maxexecutiontime=440 -threads 2" > plotter/r1.txt 2>&1 ;'

gnome-terminal -x bash -c 'sudo -u prashanth sshpass -p "hondacivic" ssh prashanth@10.129.26.195 "cd testing/ycsb; bin/ycsb.sh run redis -s -P workloads/workload2 -p redis.host=10.129.28.6 -p redis.port=3002 -p maxexecutiontime=440 -threads 2" > plotter/r2.txt 2>&1 ;'

# Start Collected
python /home/prashanth/Workspace/mem_reclamation/experiments/plotter/collector.py &
echo "Collector PID: "$!

# 40s to let processes from containers to settle in
sleep 100

# Bring in externel processes with pressure one after the other after 30s intevels
for var in 14000 12000 10000 9000 8000 7000 6000 5000 4000 3000 
do
        echo $var
        gnome-terminal -x bash -c "sudo -u prashanth sshpass -p 'hondacivic' ssh synerg@10.129.34.7 virsh setmem MTP-1 "$var"M;"
        sleep 30
done

gnome-terminal -x bash -c "sudo -u prashanth sshpass -p 'hondacivic' ssh synerg@10.129.34.7 virsh setmem MTP-1 16G;"

# 20 seconds for reclamation to reach normal
sleep 30

docker stop m1 m2 r1 r2 

# Remove Kernel Module
sudo rmmod kernel_project.ko

# Copy files from remote to local
#sshpass -p "hondacivic" scp -r prashanth@10.129.26.185:plotter .

# Output syslog to desired location
cat /var/log/syslog > /home/prashanth/Workspace/mem_reclamation/experiments/plotter/log


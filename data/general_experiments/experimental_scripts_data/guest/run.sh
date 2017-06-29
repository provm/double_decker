#dir stands for test_name
dir=$1
exp=$2

poolid_1=""
poolid_2=""
poolid_3=""
poolid_4=""

#############################################################

if [ "$#" -lt 2 ]; then
  	echo "VM-RUN: Enter correct arguments!"
  	exit 1
fi

mkdir $exp/$dir
mkdir $exp/$dir/app

if [ $? -ne 0 ] ; then
        echo "VM-RUN: Test Name already exists"
        exit 1
fi

echo "VM-RUN: Started!"

for i in 1 2 3 4;
do
   name="c$i"
   eval poolid_$i=$( cat /sys/fs/cgroup/memory/lxc/$name/hcache_poolid )
done

# Calling pool specific logger at host
sudo -u root sshpass -p synerg ssh 10.129.28.78 "cd /home/synerg/prashanth/experiment/; helper_scripts/logger.sh $poolid_1 $poolid_2 $poolid_3 $poolid_4 $dir $exp" &

# Start logger
bash helper_scripts/logger.sh $dir $exp &

# Start workloads
cd /home/vm1/clients/oltp; ./oltpbenchmark -b ycsb -c /home/vm1/clients/oltp/config/sample_ycsb_config.xml --execute=true >> /home/vm1/experiments/$exp/$dir/app/c4.out 2>&1 &
cd /home/vm1/experiments

/home/vm1/clients/ycsb_mongo/bin/ycsb run mongodb -s -P /home/vm1/clients/ycsb_mongo/workloads/workload1 -p mongodb.url=mongodb://10.0.3.220:27017/ycsb?w=0 -p "maxexecutiontime=600" >> $exp/$dir/app/c1.out 2>&1 &

/home/vm1/clients/ycsb_redis/bin/ycsb run redis -s -P /home/vm1/clients/ycsb_redis/workloads/workload1 -p "redis.host=10.0.3.168" -p "redis.port=6379" -p "operationcount=100000000" -p "maxexecutiontime=600">> $exp/$dir/app/c3.out 2>&1 &

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.176 /home/ubuntu/filebench-1.4.9.1/filebench -f /home/ubuntu/filebench-1.4.9.1/workloads/webserver.f >> $exp/$dir/app/c2.out 2>&1 &


sleep 10

op=`pgrep filebench || pgrep java`
while [ "$op" != "" ];
do
   sleep 20
   op=`pgrep filebench || pgrep java` 
done

echo "VM-RUN: Terminated! Press Ctrl+C now, to stop HOST-LOGGER."


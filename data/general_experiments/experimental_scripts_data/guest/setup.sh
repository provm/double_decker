#dir stands for test_name
dir=$1
exp=$2

poolid_1=""
poolid_2=""
poolid_3=""
poolid_4=""

#############################################################

if [ "$#" -lt 2 ]; then
  	echo "VM-SETUP: Enter correct arguments!"
  	exit 1
fi

mkdir /sys/fs/cgroup/memory/lxc
echo 1 > /sys/fs/cgroup/memory/lxc/memory.use_hierarchy

mkdir stats_setup/$dir
mkdir stats_setup/$dir/app

if [ $? -ne 0 ] ; then
        echo "VM-SETUP: Test Name already exists"
        exit 1
fi

echo "VM-SETUP: Started!"

echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/enabled
#echo 3 > /proc/sys/vm/drop_caches
echo 0 > /proc/sys/kernel/randomize_va_space

lxc-start -n c1
lxc-start -n c2
lxc-start -n c3
lxc-start -n c4

sleep 5

echo 6G > /sys/fs/cgroup/memory/lxc/memory.limit_in_bytes
echo 30G > /sys/fs/cgroup/memory/lxc/memory.memsw.limit_in_bytes

for i in 1 2 3 4;
do
   name="c$i"
   #echo 1024M > /sys/fs/cgroup/memory/lxc/$name/memory.limit_in_bytes
   #echo 5G > /sys/fs/cgroup/memory/lxc/$name/memory.memsw.limit_in_bytes

   echo 125 > /sys/fs/cgroup/blkio/lxc/$name/blkio.weight
	
   eval poolid_$i=$( cat /sys/fs/cgroup/memory/lxc/$name/hcache_poolid )
   
done
   
# DD Experiment specific config
#echo 2G > /sys/fs/cgroup/memory/lxc/c3/memory.limit_in_bytes
#echo 2G > /sys/fs/cgroup/memory/lxc/c4/memory.limit_in_bytes

# weights
echo 60 > /sys/fs/cgroup/memory/lxc/c1/hcache_weight
echo 40 > /sys/fs/cgroup/memory/lxc/c2/hcache_weight

echo 2-3 >  /sys/fs/cgroup/cpuset/lxc/c1/cpuset.cpus    #2-3 --> c1, 4-5 --> c2, 6-7->c3, 8-9->c4, 10,11,12,13->client1, client3, client4
echo 4-5 >  /sys/fs/cgroup/cpuset/lxc/c2/cpuset.cpus
echo 6-7 >  /sys/fs/cgroup/cpuset/lxc/c3/cpuset.cpus
echo 8-9 >  /sys/fs/cgroup/cpuset/lxc/c4/cpuset.cpus

sleep 30

# Calling pool specific logger at host
sudo -u root sshpass -p synerg ssh 10.129.28.78 "cd /home/synerg/prashanth/experiment/; helper_scripts/logger.sh $poolid_1 $poolid_2 $poolid_3 $poolid_4 $dir $exp" &

# Start logger
bash helper_scripts/logger.sh $dir $exp &

# Start workloads
/home/vm1/clients/ycsb_mongo/bin/ycsb run mongodb -s -P /home/vm1/clients/ycsb_mongo/workloads/workload1 -p mongodb.url=mongodb://10.0.3.220:27017/ycsb?w=0 -p "maxexecutiontime=600" >> $exp/$dir/app/c1.out 2>&1

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.176 /home/ubuntu/filebench-1.4.9.1/filebench -f /home/ubuntu/filebench-1.4.9.1/workloads/webserver.f >> $exp/$dir/app/c2.out 2>&1

/home/vm1/clients/ycsb_redis/bin/ycsb load redis -s -P /home/vm1/clients/ycsb_redis/workloads/workload1 -p "redis.host=10.0.3.168" -p "redis.port=6379" >> $exp/$dir/app/c3.out 2>&1

cd /home/vm1/clients/oltp; ./oltpbenchmark -b ycsb -c /home/vm1/clients/oltp/config/sample_ycsb_config.xml --create=true --load=true >> /home/vm1/experiments/$exp/$dir/app/c4.out 2>&1
cd /home/vm1/experiments

sleep 10

op=`pgrep filebench || pgrep java`
while [ "$op" != "" ];
do
   sleep 20
   op=`pgrep filebench || pgrep java` 
done

echo "VM-SETUP: Terminated! Press Ctrl+C now, to stop HOST-LOGGER."

#dir stands for test_name
#dir=$1
#exp=$2

#poolid_1=""
#poolid_2=""
#poolid_3=""
#poolid_4=""

#if [ "$#" -lt 2 ]; then
  	#echo "VM-SETUP: Enter correct arguments!"
  	#exit 1
#fi

#mkdir stats_setup/$dir
#mkdir stats_setup/$dir/app

#if [ $? -ne 0 ] ; then
        #echo "VM-SETUP: Test Name already exists"
        #exit 1
#fi

#echo "VM-SETUP: Started!"

echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo 3 > /proc/sys/vm/drop_caches
echo 0 > /proc/sys/kernel/randomize_va_space

lxc-start -n c1
lxc-start -n c2
lxc-start -n c3
lxc-start -n c4

sleep 5

lxc-start -n client1
lxc-start -n client3
lxc-start -n client4

#echo 6G > /sys/fs/cgroup/memory/lxc/memory.limit_in_bytes
#echo 30G > /sys/fs/cgroup/memory/lxc/memory.memsw.limit_in_bytes

for i in 1 2 3 4;
do
   name="c$i"
   echo 1536M > /sys/fs/cgroup/memory/lxc/$name/memory.limit_in_bytes
   echo 5G > /sys/fs/cgroup/memory/lxc/$name/memory.memsw.limit_in_bytes

   echo 125 > /sys/fs/cgroup/blkio/lxc/$name/blkio.weight
	
   echo 25 > /sys/fs/cgroup/memory/lxc/$name/hcache_weight
   #echo 1025 > /sys/fs/cgroup/memory/lxc/$name/hcache_weight
   
   eval poolid_$i=$( cat /sys/fs/cgroup/memory/lxc/$name/hcache_poolid )
  
done

for i in 1 3 4;
do
   name="client$i"
   echo 125 > /sys/fs/cgroup/blkio/lxc/$name/blkio.weight
done

echo 1-3 >  /sys/fs/cgroup/cpuset/lxc/c1/cpuset.cpus
echo 4 >  /sys/fs/cgroup/cpuset/lxc/client1/cpuset.cpus

echo 5-7 >  /sys/fs/cgroup/cpuset/lxc/c2/cpuset.cpus

echo 8-10 >  /sys/fs/cgroup/cpuset/lxc/c3/cpuset.cpus
echo 11 >  /sys/fs/cgroup/cpuset/lxc/client3/cpuset.cpus

echo 12-14 >  /sys/fs/cgroup/cpuset/lxc/c4/cpuset.cpus
echo 15 >  /sys/fs/cgroup/cpuset/lxc/client4/cpuset.cpus

sleep 30

# Calling pool specific logger at host

#sudo -u root sshpass -p synerg ssh 10.129.28.78 "cd /home/synerg/prashanth/experiment/; helper_scripts/logger.sh $poolid_1 $poolid_2 $poolid_3 $poolid_4 $dir $exp" &

# Start logger
#bash helper_scripts/logger.sh $dir $exp &

# Start workloads
sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.48 /home/ubuntu/ycsb/bin/ycsb run mongodb -s -P /home/ubuntu/ycsb/workloads/workload1 -p mongodb.url=mongodb://10.0.3.220:27017/ycsb?w=0 -p "maxexecutiontime=1000" >> ds.log 2>&1 &

#sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.176 /home/ubuntu/filebench-1.4.9.1/filebench -f /home/ubuntu/filebench-1.4.9.1/workloads/webserver.f >> $exp/$dir/app/c2.out 2>&1

#sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.25 /home/ubuntu/ycsb/bin/ycsb load redis -s -P /home/ubuntu/ycsb/workloads/workload1 -p "redis.host=10.0.3.168" -p "redis.port=6379" >> $exp/$dir/app/c3.out 2>&1

#sudo -u root sshpass -p ubuntu ssh ubuntu@10.0.3.217 "cd oltp; ./oltpbenchmark -b ycsb -c /home/ubuntu/oltp/config/sample_ycsb_config.xml --create=true --load=true" >> $exp/$dir/app/c4.out 2>&1

sleep 10

echo -n > mstat.out
op=`pgrep java`
while [ "$op" != "" ];
do
   sleep 30
   op=`pgrep java`
   cat /sys/fs/cgroup/memory/lxc/c1/memory.stat >> mstat.out
done

#echo "VM-SETUP: Terminated! Press Ctrl+C now, to stop HOST-LOGGER."

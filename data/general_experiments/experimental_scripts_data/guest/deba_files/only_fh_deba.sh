#!/bin/bash
echo 3 > /proc/sys/vm/drop_caches
echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo 0 > /proc/sys/kernel/randomize_va_space

#lxc-stop -n c1
#lxc-stop -n c2
#lxc-stop -n c3
#lxc-stop -n c4
#lxc-stop -n client1
#lxc-stop -n client3
#lxc-stop -n client4

#sleep 60

lxc-start -n c1
lxc-start -n c2
lxc-start -n c3
lxc-start -n c4

sleep 5

lxc-start -n client1
lxc-start -n client3
lxc-start -n client4

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

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.48 /home/ubuntu/ycsb/bin/ycsb run mongodb -s -P /home/ubuntu/ycsb/workloads/workload1 -p mongodb.url=mongodb://10.0.3.220:27017/ycsb?w=0 -p "maxexecutiontime=1000" > ds.log 2>&1 &

sleep 10

echo -n > mstat.out
op=`pgrep java`
while [ "$op" != "" ];
do
   sleep 30
   op=`pgrep java` 
   cat /sys/fs/cgroup/memory/lxc/c1/memory.stat >> mstat.out
done


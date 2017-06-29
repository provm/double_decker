sudo -u root sshpass -p synerg ssh 10.129.28.78 "echo 262144 > /sys/kernel/mm/utmem/global_limit"

echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo 3 > /proc/sys/vm/drop_caches
echo 0 > /proc/sys/kernel/randomize_va_space

lxc-start -n c1

sleep 5

lxc-start -n client1

for i in 1;
do
   name="c$i"
   echo 1536M > /sys/fs/cgroup/memory/lxc/$name/memory.limit_in_bytes
   echo 5G > /sys/fs/cgroup/memory/lxc/$name/memory.memsw.limit_in_bytes

   echo 125 > /sys/fs/cgroup/blkio/lxc/$name/blkio.weight
   
   cpu=`echo "($i - 1) * 2" | bc`
   echo 1-2 >  /sys/fs/cgroup/cpuset/lxc/$name/cpuset.cpus
done

echo 3 >  /sys/fs/cgroup/cpuset/lxc/client1/cpuset.cpus

echo 100 > /sys/fs/cgroup/memory/lxc/c1/hcache_weight

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.48 /home/ubuntu/ycsb/bin/ycsb run mongodb -s -P /home/ubuntu/ycsb/workloads/workload1 -p mongodb.url=mongodb://10.0.3.220:27017/ycsb?w=0 -p "maxexecutiontime=1000" 

sleep 10

op=`pgrep sshpass`
while [ "$op" != "" ];
do
   sleep 10
   op=`pgrep sshpass` 
done

echo "Setup Done !"


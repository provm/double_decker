sudo -u root sshpass -p synerg ssh 10.129.28.78 "echo 393216 > /sys/kernel/mm/utmem/global_limit"

echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo 3 > /proc/sys/vm/drop_caches
echo 0 > /proc/sys/kernel/randomize_va_space

lxc-start -n c3

sleep 5

lxc-start -n client3

for i in 3;
do
   name="c$i"
   echo 2G > /sys/fs/cgroup/memory/lxc/$name/memory.limit_in_bytes
   echo 5G > /sys/fs/cgroup/memory/lxc/$name/memory.memsw.limit_in_bytes

   echo 125 > /sys/fs/cgroup/blkio/lxc/$name/blkio.weight
   
   cpu=`echo "($i - 1) * 2" | bc`
   echo $i >  /sys/fs/cgroup/cpuset/lxc/$name/cpuset.cpus
done

echo 100 > /sys/fs/cgroup/memory/lxc/c3/hcache_weight

sleep 5

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.25 /home/ubuntu/ycsb/bin/ycsb load redis -s -P /home/ubuntu/ycsb/workloads/workload1 -p "redis.host=10.0.3.168" -p "redis.port=6379"

op=`pgrep sshpass`
while [ "$op" != "" ];
do
   sleep 10
   op=`pgrep sshpass` 
done

echo "Setup Done !"


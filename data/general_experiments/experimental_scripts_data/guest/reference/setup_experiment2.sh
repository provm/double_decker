echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo 3 > /proc/sys/vm/drop_caches
echo 0 > /proc/sys/kernel/randomize_va_space

lxc-start -n c1
lxc-start -n c3

sleep 5

lxc-start -n client1
lxc-start -n client3

for i in 1 3;
do
   name="c$i"
   #echo 1228M > /sys/fs/cgroup/memory/lxc/$name/memory.limit_in_bytes
   echo 5G > /sys/fs/cgroup/memory/lxc/$name/memory.memsw.limit_in_bytes

   echo 125 > /sys/fs/cgroup/blkio/lxc/$name/blkio.weight
   
   cpu=`echo "($i - 1) * 2" | bc`
   echo $i >  /sys/fs/cgroup/cpuset/lxc/$name/cpuset.cpus
done

echo 410M > /sys/fs/cgroup/memory/lxc/c1/memory.limit_in_bytes
echo 2G > /sys/fs/cgroup/memory/lxc/c3/memory.limit_in_bytes

echo 90 > /sys/fs/cgroup/memory/lxc/c1/hcache_weight
echo 10 > /sys/fs/cgroup/memory/lxc/c3/hcache_weight

echo 1090 > /sys/fs/cgroup/memory/lxc/c1/hcache_weight
echo 1010 > /sys/fs/cgroup/memory/lxc/c3/hcache_weight

echo 0 > /sys/fs/cgroup/memory/lxc/client1/hcache_weight
echo 0 > /sys/fs/cgroup/memory/lxc/client3/hcache_weight

#mongo 10.0.3.220:27017/ycsb --eval "db.dropDatabase()"

sleep 10

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.48 /home/ubuntu/ycsb/bin/ycsb run mongodb -s -P /home/ubuntu/ycsb/workloads/workload1 -p mongodb.url=mongodb://10.0.3.220:27017/ycsb?w=0 -p "maxexecutiontime=1000" &
sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.25 /home/ubuntu/ycsb/bin/ycsb load redis -s -P /home/ubuntu/ycsb/workloads/workload1 -p "redis.host=10.0.3.168" -p "redis.port=6379"

op=`pgrep sshpass`
while [ "$op" != "" ];
do
   sleep 10
   op=`pgrep sshpass` 
done

echo "Setup Done !"


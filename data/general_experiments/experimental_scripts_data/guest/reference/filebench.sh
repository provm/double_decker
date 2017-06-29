sudo -u root sshpass -p synerg ssh 10.129.28.78 "echo 0 > /sys/kernel/mm/utmem/global_limit"

echo 3 > /proc/sys/vm/drop_caches
echo 0 > /proc/sys/kernel/randomize_va_space

lxc-start -n c2
sleep 3

mkdir stats/$1
lxc-top > stats/$1

#bash log_cache_usage.sh test_name vm cont &

for i in 2;
do
   name="c$i"
   
   echo 2G > /sys/fs/cgroup/memory/lxc/$name/memory.limit_in_bytes
   #echo 1221M > /sys/fs/cgroup/memory/lxc/$name/memory.limit_in_bytes
   #echo 1G > /sys/fs/cgroup/memory/lxc/$name/memory.limit_in_bytes
   echo 5G > /sys/fs/cgroup/memory/lxc/$name/memory.memsw.limit_in_bytes

   echo 125 > /sys/fs/cgroup/blkio/lxc/$name/blkio.weight
   
   cpu=`echo "($i - 1) * 2" | bc`
   echo $i >  /sys/fs/cgroup/cpuset/lxc/$name/cpuset.cpus
done

echo 100 > /sys/fs/cgroup/memory/lxc/c2/hcache_weight

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.176 /home/ubuntu/filebench-1.4.9.1/filebench -f /home/ubuntu/filebench-1.4.9.1/workloads/webserver.f

op=`pgrep sshpass`
while [ "$op" != "" ];
do
   sleep 10
   op=`pgrep sshpass` 
done

lxc-top >> stats/$1

echo "Exp Done !"


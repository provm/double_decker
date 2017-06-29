sudo -u root sshpass -p synerg ssh 10.129.28.78 "echo 262144 > /sys/kernel/mm/utmem/global_limit"

echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo 3 > /proc/sys/vm/drop_caches
echo 0 > /proc/sys/kernel/randomize_va_space

lxc-start -n c4

sleep 5

lxc-start -n client4

for i in 4;
do
   name="c$i"
   echo 512M > /sys/fs/cgroup/memory/lxc/$name/memory.limit_in_bytes
   echo 5G > /sys/fs/cgroup/memory/lxc/$name/memory.memsw.limit_in_bytes

   echo 125 > /sys/fs/cgroup/blkio/lxc/$name/blkio.weight
   
   cpu=`echo "($i - 1) * 2" | bc`
   #echo $i >  /sys/fs/cgroup/cpuset/lxc/$name/cpuset.cpus
done

echo 100 > /sys/fs/cgroup/memory/lxc/c4/hcache_weight

#sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.43 oltpbench-master/oltpbenchmark -b ycsb -c oltpbench-master/config/sample_tpcc_config.xml --execute=true -s 5 

#sleep 10

#op=`pgrep sshpass`
#while [ "$op" != "" ];
#do
   #sleep 10
   #op=`pgrep sshpass` 
#done

#echo "MySQL Setup Done !"


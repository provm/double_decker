if [ "$#" -lt 1 ]; then
  	echo "ERROR: Enter Test Name !"
  	exit 1
fi

dir=$1

mkdir stats/$dir

if [ $? -ne 0 ] ; then
    	echo "Test Name already exists"
	exit 1
fi

mkdir stats/$dir/memcg
mkdir stats/$dir/app

for i in 1 2 3 4;
do
   name="c$i"
   echo "========== START: Container $i Memory STATS ==============" > stats/$dir/memcg/c$i.out
   cat /sys/fs/cgroup/memory/lxc/$name/memory.stat >> stats/$dir/memcg/c$i.out
   echo "======== END Container STATS ======" >> stats/$dir/memcg/c$i.out
done

lxc-top > stats/$dir/top.out

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.48 /home/ubuntu/ycsb/bin/ycsb run mongodb -s -P /home/ubuntu/ycsb/workloads/workload1 -p mongodb.url=mongodb://10.0.3.220:27017/ycsb?w=0 -p "maxexecutiontime=600">> stats/$dir/app/c1.out 2>&1 &

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.176 /home/ubuntu/filebench-1.4.9.1/filebench -f /home/ubuntu/filebench-1.4.9.1/workloads/webserver.f >> stats/$dir/app/c2.out 2>&1 &

sudo -u vm1 sshpass -p ubuntu ssh ubuntu@10.0.3.25 /home/ubuntu/ycsb/bin/ycsb run redis -s -P /home/ubuntu/ycsb/workloads/workload1 -p "redis.host=10.0.3.168" -p "redis.port=6379" -p "operationcount=100000000" -p "maxexecutiontime=600">> stats/$dir/app/c3.out 2>&1 &

sudo -u root sshpass -p ubuntu ssh ubuntu@10.0.3.217 "cd oltp; ./oltpbenchmark -b ycsb -c /home/ubuntu/oltp/config/sample_ycsb_config.xml --execute=true" >> stats/$dir/app/c4.out 2>&1 &

sleep 10

op=`pgrep sshpass`
while [ "$op" != "" ];
do
   sleep 10
   op=`pgrep sshpass` 
done


for i in 1 2 3 4;
do
   name="c$i"
   echo "========== STOP: Container $i Memory STATS ==============" >> stats/$dir/memcg/c$i.out
   cat /sys/fs/cgroup/memory/lxc/$name/memory.stat >> stats/$dir/memcg/c$i.out
   echo "======== END Container STATS ======" >> stats/$dir/memcg/c$i.out
done

lxc-top >> stats/$dir/top.out

echo "RUN Done !"

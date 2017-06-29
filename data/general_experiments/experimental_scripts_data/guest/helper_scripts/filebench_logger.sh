if [ "$#" -lt 2 ]; then
        echo "VM-LOGGER: Enter Test Name !"
        exit 1
fi


#echo $1, $2, $3, $4, $5, $6

#dir refers to experiment name 
dir=$1

vm="0"
exp=$2

mkdir $exp/$dir/cgroup

if [ $? -ne 0 ] ; then
        echo "VM-LOGGER: Logger stats already exist for this test exist"
        #exit 1
fi

echo "VM-LOGGER: Started!"

sleep 5

op=`pgrep filebench || pgrep java`
while [ "$op" != "" ];
do
        for i in 2;
        do
                cont="c$i"

		date >> $exp/$dir/cgroup/$cont.out
		
		echo "--------------------MEMCG-----------------------" >>  $exp/$dir/cgroup/$cont.out
		echo "MEM ONLY  : $((`cat /sys/fs/cgroup/memory/lxc/$cont/memory.usage_in_bytes`/1048576)) MB" >> $exp/$dir/cgroup/$cont.out
		echo "MEM+SWAP  : $((`cat /sys/fs/cgroup/memory/lxc/$cont/memory.memsw.usage_in_bytes`/1048576)) MB" >> $exp/$dir/cgroup/$cont.out
		cat /sys/fs/cgroup/memory/lxc/$cont/memory.stat >> $exp/$dir/cgroup/$cont.out
                
		echo "--------------------BLKIO-----------------------" >>  $exp/$dir/cgroup/$cont.out
		cat /sys/fs/cgroup/blkio/lxc/$cont/blkio.throttle.io_service_bytes >>  $exp/$dir/cgroup/$cont.out
				
		echo "--------------------CPUAC-----------------------" >>  $exp/$dir/cgroup/$cont.out
		cat /sys/fs/cgroup/cpuacct/lxc/$cont/cpuacct.stat >>  $exp/$dir/cgroup/$cont.out

		echo " " >> $exp/$dir/cgroup/$cont.out

        done

	sleep 30
        op=`pgrep filebench || pgrep java`
done

echo "VM-LOGGER: Terminated!"

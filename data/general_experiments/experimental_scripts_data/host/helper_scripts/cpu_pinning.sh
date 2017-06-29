for i in $(seq 0 5);
do	
	virsh vcpupin pp1 $i $(($i+2))
done

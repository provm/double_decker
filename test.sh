#make
sudo insmod cgtmem.ko
sleep 1
virsh start pp1
sleep 5
echo 786432 > /sys/kernel/mm/utmem/global_limit
echo 0 > /sys/kernel/mm/ksm/run

sleep 50

echo 100 > /sys/kernel/mm/utmem/0/weight

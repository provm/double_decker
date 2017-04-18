#make
sudo insmod cgtmem.ko
sleep 1
virsh start vm2
sleep 5
echo 524288 > /sys/kernel/mm/utmem/global_limit
echo 0 > /sys/kernel/mm/ksm/run

sleep 30

echo 100 > /sys/kernel/mm/utmem/0/weight

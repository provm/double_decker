#make
sudo insmod cgtmem.ko
sleep 1
virsh start vm2
sleep 5
echo 76800 > /sys/kernel/mm/utmem/global_limit
echo 100 > /sys/kernel/mm/utmem/0/weight

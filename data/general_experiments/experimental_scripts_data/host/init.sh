gb=$((1024 * 256))

vm1_weight=100

mem_limit=$((2 * $gb))
ssd_limit=$((10 * $gb))

tmp=$(virsh list --all | grep "pp1" | awk '{ print $3}')

if ([ "x$tmp" == "x" ] || [ "x$tmp" != "xrunning" ])
then
    echo "Setting initial configuration"
else
    echo "HOST-INIT: ERROR! VM is running! Poweroff vm before stating script"
    exit 1
fi

echo 0 > /sys/kernel/mm/ksm/run

insmod ../tmem_module-5.0/cgtmem.ko
virsh start pp1

echo $mem_limit > /sys/kernel/mm/utmem/global_limit
echo $ssd_limit > /sys/kernel/mm/utmem/ssd_limit

sleep 40

bash helper_scripts/cpu_pinning.sh
bash helper_scripts/fix_cpu_freq.sh
echo $vm1_weight > /sys/kernel/mm/utmem/0/weight

echo "HOST-INIT: Terminated !"

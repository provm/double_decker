if [ "$#" -lt 3 ]; then
        echo "HOST-SETUP: <mem-cache-size> <ssd-cache-size> <test-name>"
        exit 1
fi

mb=$((256))

vm1_mem_weight=100
vm1_ssd_weight=1100

mem_limit=$(($1 * $mb))
ssd_limit=$(($2 * $mb))

test_name=$3
experiment_type=stats_mongo

######################################################

echo $vm1_mem_weight > /sys/kernel/mm/utmem/0/weight
echo $vm1_ssd_weight > /sys/kernel/mm/utmem/0/weight

echo $mem_limit > /sys/kernel/mm/utmem/global_limit
echo $ssd_limit > /sys/kernel/mm/utmem/ssd_limit

mkdir $experiment_type/$test_name

echo "HOST-SETUP: Started!"

sudo -u root sshpass -p 1234 ssh 192.168.122.190 "cd /home/vm1/experiments/; bash mongo_setup.sh $test_name $experiment_type;"

echo "HOST-SETUP: Terminated!"

mb=$((256))

vm1_weight=100

mem_limit=$((1536 * $mb))
ssd_limit=$((0 * $mb))

test_name=$1
experiment_type=stats_redis

######################################################

echo $mem_limit > /sys/kernel/mm/utmem/global_limit
echo $ssd_limit > /sys/kernel/mm/utmem/ssd_limit

mkdir $experiment_type/$test_name

if [ "$#" -lt 1 ]; then
        echo "HOST-SETUP: Enter Test Name !"
        exit 1
fi

echo "HOST-SETUP: Started!"

sudo -u root sshpass -p 1234 ssh 192.168.122.190 "cd /home/vm1/experiments/; bash redis_setup.sh $test_name $experiment_type;"

#sleep 10
#bash helper_scripts/logger.sh $test_name $experiment_type

echo "HOST-SETUP: Terminated!"

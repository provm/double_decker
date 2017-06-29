gb=$((1024 * 256))

vm1_weight=100

test_name=$1
experiment_type=stats_run

######################################################

mkdir $experiment_type/$test_name

if [ "$#" -lt 1 ]; then
        echo "HOST-RUN: Enter Test Name !"
        exit 1
fi

echo "HOST-RUN: Started!"

sudo -u root sshpass -p 1234 ssh 192.168.122.190 "cd /home/vm1/experiments/; bash run.sh $test_name $experiment_type;"

echo "HOST-RUN: Terminated!"

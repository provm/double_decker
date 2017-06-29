if [ "$#" -lt 6 ]; then
        echo "HOST-LOGGER: Not enough parameters !"
        exit 1
fi


echo $1, $2, $3, $4, $5, $6

#dir refers to experiment name 
dir=$5

vm="0"
exp=$6

mkdir $exp/$dir/utmem
mkdir $exp/$dir/cache

if [ $? -ne 0 ] ; then
        echo "HOST-LOGGER: Logger stats already exist for this test exist"
        exit 1
fi

echo "HOST-LOGGER: Started!"

op=`pgrep sshpass`
#op="123213"

while [ "$op" != "" ];
do
        sleep 30

        for i in $1 $2 $3 $4;
        do
                cont=$i

		date >> $exp/$dir/utmem/$cont.out                                                    
                cat /sys/kernel/mm/utmem/$vm/$cont/stats >> $exp/$dir/utmem/$cont.out
                echo "---------------------------------------------" >> $exp/$dir/utmem/$cont.out                
                echo "$(date) = $(cat /sys/kernel/mm/utmem/$vm/$cont/mem_used)" >>  $exp/$dir/cache/$cont.out 		
        done

        op=`pgrep sshpass`
done

echo "HOST-LOGGER: Terminated!"

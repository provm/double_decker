dir=$1
vm=$2
cont=$3

mkdir stats/$dir

sudo -u root sshpass -p synerg ssh synerg@10.129.28.78 cat /sys/kernel/mm/utmem/$vm/$cont/stats > stats/$dir/utmem.out

op=`pgrep sshpass`
while [ "$op" != "" ];
do
   sleep 1
  
   sudo -u root sshpass -p synerg ssh synerg@10.129.28.78 cat /sys/kernel/mm/utmem/$vm/$cont/mem_used >> stats/$dir/cache_used
   
   op=`pgrep sshpass` 
done

sudo -u root sshpass -p synerg ssh synerg@10.129.28.78 cat /sys/kernel/mm/utmem/$vm/$cont/stats >> stats/$dir/utmem.out

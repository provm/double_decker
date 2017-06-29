tmp=`virsh list`

echo $tmp

if ([ "x$tmp" == "x" ] || [ "x$tmp" != "xrunning" ])
then
    echo "VM does not exist or is shut down!"
    # Try additional commands here...
else
    echo "VM is running!"
fi

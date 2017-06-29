This directory contains the code to our double_decker implementation.

./module		Contains the kernel loadable module implementation of our backed, use Makefile to compile
./patch	
	./host.patch	Kernel patch to be applied at the host (kernel version 4.1.3)
	./guest.patch	Kernel patch to be applied at the guest (kernel version 4.1.3)

_________________

DEPENDENCIES

- Linux Kernel 4.1.3 both on guest and host
- KVM hypervisor at host
- LXC containers at guest
- Enable the following boot parameters at the guest kernel "cgtmem swapaccount=1"

_________________

LAST MACHINE USED

Location:	Synerg Lab, below deba's desk
SSH Login:	deba@10.129.78.110
Password:	contact me (kvmprashanth@gmail.com) or deba

Guest-IP:	vm1@192.168.122.190 (pp1)
_________________

WORKLOADS  

Configured as c1, c2, c3, c4 respectively in the Guest-VM above (pp1).
YCSB clients for the same are configured both within the VM in directories, as well
as containers at client1, client3, client4

1. MongoDB
2. Webserver
3. Redis
4. MySQL

Client used for 1,3,4 above is YCSB. YCSB can be found at https://github.com/brianfrankcooper/YCSB

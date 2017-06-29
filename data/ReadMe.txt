Directory contains scripts, data and plots related to MTP.

./general_experiments	Anything related to double decker experimentation
./middleware		Plots and data collected for middleware

___________________________

./general_experiments/experimental_scripts_data 	Both host+guest stats make complete exp stats
	./host		
		Contains scripts and data collected at the host. 
		
		init.sh				Used to load modules and start VM1
		setup.sh			To heat all 4 workloads before actual run
		run.sh				Is the actual script used to run
		stop.sh				Stops the containers running
		full_stop.sh			Stops containers and shutsdown VM

		<workload_name>_setup.sh 	To setup the workload	
		<workload_name>_run.sh 		To run the workload after setup (if doesn't exist use setup itself)
		
		./stats_setup			Dir used to store host related stats during setup of 4 workloads
		./stats_run			Dir used to store host related stats during run of 4 workloads

		./stats_<workload>		Stats related the individual workloads
	./guest
		Contains scripts and data collected at the guest
		Directory structure description is same as above

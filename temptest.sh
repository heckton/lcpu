#!/bin/bash

# Monitor temperature at different loads

n_print=4 # Number of printing temperatures
sleep_time=5 # Number of seconds between loads

sum_temp=0
n_temp=0

# Show throttling
dmesg -We &
pid_dmesg=$!

for threads in 1 4 8 1 4 8; do
	echo
	for time in 0.1 0.5 1 5; do
		echo
		for memory in 256*1024 6*1024*1024; do
			echo
			echo "Threads: $threads; Time: $time; Memory: $memory;"
			./lcpu $threads $time $memory &
			pid_lcpu=$!

			# Show temperature
			echo "Temperatures:"
			interval=$(echo "$time/$n_print" | bc -l)
			for i in $(seq 1 $n_print); do
				sleep $interval
				temp=$(< /sys/class/thermal/thermal_zone0/temp)
				let sum_temp=$sum_temp+$temp
				let n_temp=$n_temp+1
				echo -e "\t$temp"
			done

			wait $pid_lcpu
			# Wait raise of frequency
			sleep $sleep_time
		done
	done
done

kill $pid_dmesg

let average_temp=$sum_temp/$n_temp
echo -e "\nAverage: $average_temp"

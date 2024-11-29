#!/bin/bash

# Compile the simulator
g++ -std=c++11 interrupts.cpp -o simulator

# Test Case 1: FCFS Scheduler
echo "Running Test Case 1: FCFS Scheduler"

# Create input_data_fcfs.txt
cat <<EOT > input_data_fcfs.txt
15, 10, 0, 25, 11, 3
12, 1, 0, 50, 10, 5
1, 10, 0, 100, 25, 25
2, 1, 5, 20, 10, 2
3, 7, 14, 5, 2, 10
EOT

# Run the simulator with FCFS scheduler
./simulator input_data_fcfs.txt FCFS

# Save the outputs
mv execution.txt execution_fcfs.txt
mv memory_status.txt memory_status_fcfs.txt

# Test Case 2: Priority Scheduler
echo "Running Test Case 2: Priority Scheduler"

# Create input_data_priority.txt
cat <<EOT > input_data_priority.txt
15, 10, 0, 25, 11, 3, 2
12, 1, 0, 50, 10, 5, 1
1, 10, 0, 100, 25, 25, 3
2, 1, 5, 20, 10, 2, 4
3, 7, 14, 5, 2, 10, 2
EOT

# Run the simulator with Priority scheduler
./simulator input_data_priority.txt Priority

# Save the outputs
mv execution.txt execution_priority.txt
mv memory_status.txt memory_status_priority.txt

# Test Case 3: Round Robin Scheduler
echo "Running Test Case 3: Round Robin Scheduler"

# Create input_data_rr.txt
cat <<EOT > input_data_rr.txt
15, 10, 0, 25, 11, 3
12, 1, 0, 50, 10, 5
1, 10, 0, 100, 25, 25
2, 1, 5, 20, 10, 2
3, 7, 14, 5, 2, 10
EOT

# Recompile the simulator (in case of changes)
g++ -std=c++11 interrupts.cpp -o simulator

# Run the simulator with Round Robin scheduler
./simulator input_data_rr.txt RR

# Save the outputs
mv execution.txt execution_rr.txt
mv memory_status.txt memory_status_rr.txt

echo "Tests completed."
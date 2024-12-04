#!/bin/bash

# Compile the simulator
g++ -std=c++11 interrupts_101206884_101211245.cpp -o simulator

# Run the simulator with FCFS scheduler
./simulator input_data_2.txt FCFS
# Save the outputs
mv execution.txt execution_101206884_101211245.txt
mv memory_status.txt memory_status_101206884_101211245.txt
#!/bin/bash

g++ -std=c++11 interrupts_101206884_101211245.cpp -o simulator

# run the simulator with FCFS scheduler
./simulator input_data_1.txt FCFS

# you can also run the simulator with EP or RR scheduler
# ./simulator input_data_1.txt EP
# ./simulator input_data_1.txt RR

# save the outputs
mv execution.txt execution_101206884_101211245.txt
mv memory_status.txt memory_status_101206884_101211245.txt
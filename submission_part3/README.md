#### This is test1.sh
```
#!/bin/bash

g++ -std=c++11 interrupts_101206884_101211245.cpp -o simulator

# run the simulator with FCFS scheduler
./simulator input_data_1.txt FCFS
# or
# ./simulator input_data_1.txt EP # for External Priority
# or
# ./simulator input_data_1.txt RR # for Round Robin

# save the outputs
mv execution.txt execution_101206884_101211245.txt
mv memory_status.txt memory_status_101206884_101211245.txt
```

#### To execute the code for PART 1 run (this will run input_data from ex1 with FCFS):
```
sh test_1.sh
```

#### To execute the code for PART 2:
```
gcc -o semaphore Part2_C_101206884_101211245.c -lpthread
./semaphore
```
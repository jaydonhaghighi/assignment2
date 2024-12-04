#!/bin/bash

rm -f TA1.txt TA2.txt TA3.txt TA4.txt TA5.txt 

python3 db_maker.py

gcc -o marking_program Part2_A_101206884_101211245.c -lpthread
./marking_program
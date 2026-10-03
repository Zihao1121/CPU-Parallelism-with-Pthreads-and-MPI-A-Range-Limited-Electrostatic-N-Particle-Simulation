# Project name:CPU Parallelism with Pthreads and MPI
# by Zihao Gong,1005036916

## Check c++ environment & MPI environment
# run these two commands  in the command prompt, if you got output then good.
    g++ --version
    set msmpi

## unzip the file and enter that directory.
    cd "where you unzip\assg1"

    
## Build the cpp file.
    g++ assg1.cpp  -o assg1 -O3 -fopenmp -l msmpi -L MSMPI_LIB64 -I MSMPI_INC
# MSMPI_LIB64&MSMPI_INC can be found by command "set msmpi"
# on my pc, the command will be: g++ assg1.cpp  -o assg1 -O3 -fopenmp -l msmpi -L "C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" -I "C:\Program Files\ (x86)\Microsoft SDKs\MPI\Include"

# if build failed said "mpi.h: No such file or directory", pls delete line 25: #include <mpi.h>
# and add new line #include "MSMPI_INC\mpi.h".
# on my pc, the new line is at line 26 and added as comment. 
# // #include "C:\Program Files (x86)\Microsoft SDKs\MPI\Include\mpi.h"
    

## run the program
    .\assg1 <mode> <radius> <thread number> 
    mpiexec -n <process number> assg1 <mode> <radius> <thread number> <process number>

# .\assg1 is good for mode 1 and mode 2.
# mpiexec is good for all mode. So for the convenient, I only use mpiexec -n <process number> assg1 for the example.

## Reproduce Mode 1 test case
    mpiexec -n 1 assg1 1 1
    mpiexec -n 1 assg1 1 100
    mpiexec -n 1 assg1 1 1000
    mpiexec -n 1 assg1 1 5000
    mpiexec -n 1 assg1 1 10000
    mpiexec -n 1 assg1 1 13000
    mpiexec -n 1 assg1 1 18000

## Reproduce Mode 2 test case
    mpiexec -n 1 assg1 2 18000 1
    mpiexec -n 1 assg1 1 18000 2
    ...
    mpiexec -n 1 assg1 1 18000 15
    mpiexec -n 1 assg1 1 18000 16

# here is the test case to find the best combination
    mpiexec -n 1 assg1 1 17000 16
    mpiexec -n 1 assg1 1 17700 16
    mpiexec -n 1 assg1 1 17500 16
    mpiexec -n 1 assg1 1 17300 16
    mpiexec -n 1 assg1 1 17400 16
    mpiexec -n 1 assg1 1 17450 16
    mpiexec -n 1 assg1 1 17430 16

## Reproduce Mode 3 test case
    #To run mode 3, we must make sure -n <value> the value equal to the last input parameter.
    mpiexec -n 1 assg1 1 17500 16 1
    mpiexec -n 2 assg1 1 17500 8  2
    mpiexec -n 3 assg1 1 17500 4  3
    mpiexec -n 4 assg1 1 17500 15 4
    mpiexec -n 1 assg1 1 18000 16 1
    mpiexec -n 1 assg1 1 17430 16 1
    mpiexec -n 1 assg1 1 17450 16 1

## Reproduce Bonus test case:
# First build the bonus program
    g++ bonus.cpp  -o bonus -O3 -fopenmp -l msmpi -L MSMPI_LIB64 -I MSMPI_INC
# most of the independent parameters are not available for user change.
# To change cell_size, it's at bonus.cpp line 775.
# To change max_distance,desired_count, go line 433.
    mpiexec -n 1 bonus 1 45000
    mpiexec -n 1 bonus 2 45000 16
    mpiexec -n 1 bonus 3 45000 16 1

# change desired_count to 8000
    mpiexec -n 1 bonus 1 45000
    mpiexec -n 1 bonus 2 45000 16
    mpiexec -n 1 bonus 3 45000 16 1

# To reproduce the compared non-optimize code,
# go the assg1.cpp, comment line 676 and 677, uncomment line 678 and 679.
    mpiexec -n 1 assg1 1 26000 16 
    mpiexec -n 1 assg1 2 26000 16 1
    mpiexec -n 1 assg1 3 26000 16 1 1
    mpiexec -n 1 assg1 1 30000 16 
    mpiexec -n 1 assg1 2 30000 16 1
    mpiexec -n 1 assg1 3 30000 16 1 1

## Plotting scripts
    Plotting can be found in the file graph.xlsx
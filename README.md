# CPU Parallelism with Pthreads and MPI

**Author:** Zihao Gong  
**Student ID:** 1005036916

## 1. Prerequisites

This project requires:

- A C++ compiler (`g++`)
- Microsoft MPI (MS-MPI)
- MS-MPI SDK

Open Windows Command Prompt and verify your environment:

```bat
g++ --version
set msmpi
```

The commands should display the compiler version and MS-MPI environment variables.

## 2. Project Setup

Extract the project archive, then navigate to the `assg1` directory:

```bat
cd "path\to\assg1"
```

## 3. Build

Compile the main program:

```bat
g++ assg1.cpp -o assg1 -O3 -fopenmp -l msmpi -L "%MSMPI_LIB64%" -I "%MSMPI_INC%"
```

You can find the values of `MSMPI_LIB64` and `MSMPI_INC` by running:

```bat
set msmpi
```

Alternatively, specify the paths directly:

```bat
g++ assg1.cpp -o assg1 -O3 -fopenmp -l msmpi -L "C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" -I "C:\Program Files (x86)\Microsoft SDKs\MPI\Include"
```

### Troubleshooting: `mpi.h` Not Found

If compilation fails with:

```text
mpi.h: No such file or directory
```

Verify that the include path points to the directory containing `mpi.h`.

As an alternative, replace the following include at line 25:

```cpp
#include <mpi.h>
```

with the absolute path to your local MPI header:

```cpp
#include "C:\Program Files (x86)\Microsoft SDKs\MPI\Include\mpi.h"
```

A commented example is provided at line 26 of `assg1.cpp`.

## 4. Run

### Direct Execution

Suitable for Modes 1 and 2:

```bat
.\assg1 <mode> <radius> <thread_count>
```

### MPI Execution

Suitable for all modes:

```bat
mpiexec -n <process_count> assg1 <mode> <radius> <thread_count> <process_count>
```

| Parameter | Description |
| --- | --- |
| `mode` | Execution mode |
| `radius` | Radius used by the program |
| `thread_count` | Number of threads |
| `process_count` | Number of MPI processes |

The examples below use `mpiexec` consistently. Some modes require fewer arguments, as shown in the test cases.

> **Mode 3:** The value supplied to `mpiexec -n` must match the final process-count argument.

## 5. Reproduce Test Cases

### Mode 1

```bat
mpiexec -n 1 assg1 1 1
mpiexec -n 1 assg1 1 100
mpiexec -n 1 assg1 1 1000
mpiexec -n 1 assg1 1 5000
mpiexec -n 1 assg1 1 10000
mpiexec -n 1 assg1 1 13000
mpiexec -n 1 assg1 1 18000
```

### Mode 2

> **Check before running:** Several commands below use `mode = 1`, although this section is labeled Mode 2. Confirm the intended mode in the source code.

Test thread counts from 1 to 16. Selected commands are shown below:

```bat
mpiexec -n 1 assg1 2 18000 1
mpiexec -n 1 assg1 1 18000 2
mpiexec -n 1 assg1 1 18000 15
mpiexec -n 1 assg1 1 18000 16
```

#### Parameter Tuning

Use the following cases to find the best parameter combination:

```bat
mpiexec -n 1 assg1 1 17000 16
mpiexec -n 1 assg1 1 17700 16
mpiexec -n 1 assg1 1 17500 16
mpiexec -n 1 assg1 1 17300 16
mpiexec -n 1 assg1 1 17400 16
mpiexec -n 1 assg1 1 17450 16
mpiexec -n 1 assg1 1 17430 16
```

### Mode 3

> **Check before running:** The commands below use `mode = 1`, although this section is labeled Mode 3. Confirm the intended mode in the source code.

Ensure that `-n` matches the final process-count argument:

```bat
mpiexec -n 1 assg1 1 17500 16 1
mpiexec -n 2 assg1 1 17500 8 2
mpiexec -n 3 assg1 1 17500 4 3
mpiexec -n 4 assg1 1 17500 15 4
mpiexec -n 1 assg1 1 18000 16 1
mpiexec -n 1 assg1 1 17430 16 1
mpiexec -n 1 assg1 1 17450 16 1
```

## 6. Bonus Program

### Build

```bat
g++ bonus.cpp -o bonus -O3 -fopenmp -l msmpi -L "%MSMPI_LIB64%" -I "%MSMPI_INC%"
```

### Configuration

Most parameters are configured in the source code rather than through command-line arguments.

| Parameter | Location in `bonus.cpp` |
| --- | --- |
| `cell_size` | Line 775 |
| `max_distance` | Line 433 |
| `desired_count` | Line 433 |

Line numbers refer to the original source version.

### Default Test Cases

```bat
mpiexec -n 1 bonus 1 45000
mpiexec -n 1 bonus 2 45000 16
mpiexec -n 1 bonus 3 45000 16 1
```

### Test with `desired_count = 8000`

Set `desired_count` to `8000`, rebuild the program, and run:

```bat
mpiexec -n 1 bonus 1 45000
mpiexec -n 1 bonus 2 45000 16
mpiexec -n 1 bonus 3 45000 16 1
```

## 7. Comparison with the Unoptimized Implementation

To reproduce the unoptimized implementation:

1. Open `assg1.cpp`.
2. Comment out lines 676–677.
3. Uncomment lines 678–679.
4. Rebuild the program.
5. Run the following test cases:

```bat
mpiexec -n 1 assg1 1 26000 16
mpiexec -n 1 assg1 2 26000 16 1
mpiexec -n 1 assg1 3 26000 16 1 1

mpiexec -n 1 assg1 1 30000 16
mpiexec -n 1 assg1 2 30000 16 1
mpiexec -n 1 assg1 3 30000 16 1 1
```

## 8. Results and Plots

Plotting data and charts are available in [`graph.xlsx`](graph.xlsx).

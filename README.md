# Multithreaded OS Simulator

A comprehensive operating system simulator that demonstrates various CPU scheduling algorithms in a multithreaded environment. This project simulates process execution, I/O operations, and context switching to provide insights into how different scheduling policies affect system performance.

## Overview

This simulator creates a realistic environment for testing and comparing CPU scheduling algorithms. It features:

- **Multithreaded Architecture**: Uses pthreads to simulate multiple CPUs running concurrently
- **Multiple Scheduling Algorithms**: Implements FCFS, Priority Aging, Round Robin, and Shortest Remaining Time First
- **Process Simulation**: Simulates realistic processes with alternating CPU and I/O bursts
- **Real-time Visualization**: Displays a Gantt chart showing process execution over time
- **Performance Metrics**: Tracks context switches, CPU utilization, and process completion times

## Features

### Supported Scheduling Algorithms

1. **FCFS (First-Come, First-Served)**
   - Non-preemptive scheduling
   - Processes are executed in order of arrival
   - Simple but may lead to convoy effect

2. **PA (Priority Aging)**
   - Preemptive priority-based scheduling
   - Implements priority aging to prevent starvation
   - Higher priority processes can preempt lower priority ones

3. **RR (Round Robin)**
   - Preemptive scheduling with time quantum
   - Fair distribution of CPU time
   - Good for time-sharing systems

4. **SRTF (Shortest Remaining Time First)**
   - Preemptive scheduling based on remaining burst time
   - Optimal for minimizing average waiting time
   - Requires knowledge of future burst times

### Process Simulation

The simulator creates realistic processes with:
- Alternating CPU and I/O bursts
- Different arrival times
- Varying burst lengths and priorities
- Process state management (NEW, READY, RUNNING, WAITING, TERMINATED)

### Real-time Output

- **Gantt Chart**: Visual representation of process execution across CPUs
- **Statistics**: Context switches, CPU utilization, and completion times
- **Process States**: Real-time display of process states and transitions

## Building the Project

### Prerequisites

- GCC compiler with C99 support
- pthread library (usually included with GCC)
- Make utility

### Compilation

```bash
# Build with release optimizations (default)
make

# Build with debug symbols
make debug

# Build with thread sanitizer for debugging
make tsan-debug

# Clean build artifacts
make clean
```

The project compiles with strict warning flags and optimization for release builds.

## Running the Simulator

```bash
# Run with 4 CPUs (default)
./os-sim

# Run with different number of CPUs (1-16)
./os-sim [cpu_count]
```

### Example Output

The simulator provides real-time output showing:
- Gantt chart of process execution
- Current process states
- CPU utilization
- Final statistics including:
  - Total context switches
  - Average waiting time
  - CPU utilization percentage

## Project Structure

```
├── src/
│   ├── os-sim.c      # Main simulator implementation
│   ├── os-sim.h      # Simulator API and data structures
│   ├── scheduler.c   # Scheduling algorithm implementations
│   ├── scheduler.h   # Scheduler interface and queue structures
│   ├── process.c     # Process definitions and operations
│   └── process.h     # Process data structures
├── Makefile          # Build configuration
└── README.md         # This file
```

## Key Components

### Process Control Block (PCB)
- Process ID, name, and state
- CPU burst time remaining
- Priority and arrival time
- Program counter for operation simulation

### Ready Queue
- Linked list implementation
- Supports enqueue/dequeue operations
- Maintains process ordering based on scheduling algorithm

### CPU Threads
- Each CPU runs in its own thread
- Simulates process execution with timing
- Handles context switching and preemption

### I/O Simulation
- FIFO I/O queue
- Processes block during I/O operations
- Realistic I/O burst timing

## Scheduling Algorithm Details

### FCFS Implementation
- Non-preemptive scheduling
- Processes execute in arrival order
- Simple queue management

### Priority Aging
- Priority-based scheduling with aging
- Prevents starvation by increasing priority over time
- Preemptive execution

### Round Robin
- Time quantum-based scheduling
- Fair CPU time distribution
- Preemptive with fixed time slices

### SRTF
- Optimal for minimizing waiting time
- Requires knowledge of remaining burst times
- Preemptive scheduling

## Performance Analysis

The simulator helps analyze:
- **CPU Utilization**: How efficiently CPUs are used
- **Context Switch Overhead**: Impact of scheduling decisions
- **Process Completion Times**: Total time from arrival to completion
- **Fairness**: Distribution of CPU time among processes

## License

This project is available for educational and research purposes. 


/*
 * os-sim.h
 *
 * The simulator library.
 *
 */

#pragma once

/*
 * The process_state_t enum contains the possible states for a process.
 *
 */
typedef enum
{
    PROCESS_NEW = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED
} process_state_t;

typedef enum
{
    OP_CPU = 0,
    OP_IO,
    OP_TERMINATE
} op_type;

typedef struct
{
    op_type type;
    unsigned int time;
} op_t;

/*
 * The Process Control Block
 *
 *   pid  : The Process ID, a unique number identifying the process. (read-only)
 *
 *   name : A string containing the name of the process. (read-only)
 *
 *   time_in_CPU_burst: If a process is in the ready queue, this is the amount of time left in the next CPU burst. 
 *                      If a process is scheduled, it represents the amount of time left in the current burst.
 *
 *   state : The current state of the process.
 *
 *   pc : The "program counter" of the process.  This value is used
 *        by the simulator to simulate the process.
 *
 *   next : An unused pointer to another PCB.
 * 
 *   enqueue_time : The time at which the process was enqueued.  This value
 *        is used by the priority aging algorithm.
 * 
 *   total_time_remaining: The total amount of CPU and IO time left for this process
 */
typedef struct _pcb_t
{
    const unsigned int pid;
    const char *name;
    unsigned int time_in_CPU_burst;
    unsigned int priority;
    process_state_t state;
    op_t *pc;
    struct _pcb_t *next;
    unsigned int enqueue_time;
    unsigned int arrival_time; // The time at which an applicaiton is launched (Also when the process first enters the ready queue)
    unsigned int total_time_remaining;
} pcb_t;

/*
 * start_simulator() runs the OS simulation.  The number of CPUs (1-16) is
 * passed as the parameter.
 */
extern void start_simulator(unsigned int cpu_count);

/*
 * context_switch() schedules a process on a CPU.  Note that it is
 * non-blocking.  It does not actually simulate the execution of the process;
 * it simply selects the process to simulate next.
 *
 *       cpu_id : the # of the CPU on which to execute the process
 *          pcb : a pointer to the process's PCB
 *   time_slice : an integer containing the time slice to allocate to the
 *                process (in ticks--1/10th sec.).  Use -1 to give a process an
 *                infinite time slice (for FCFS scheduling).
 */
extern void context_switch(unsigned int cpu_id, pcb_t *pcb,
                           int preemption_time);

/*
 * force_preempt() preempts a running process before its timeslice expires.
 * Used by the Priority scheduler to preempt lower
 * priority processes so that higher priority processes may execute.
 */
extern void force_preempt(unsigned int cpu_id);

/*
 * mt_safe_usleep() is a thread-safe implementation of the usleep() function.
 */
extern void mt_safe_usleep(long usec);

/*
 * get_current_time() returns the current simulator time.  This function
 * is thread-safe.
 */
extern unsigned int get_current_time(void);

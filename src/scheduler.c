/*
 * scheduler.c
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "scheduler.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);

static unsigned int cpu_count;

/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] is updated by schedule() each time a process is scheduled
 * on a CPU. Since the current[] array is accessed by multiple threads, it
 * needs to use a mutex to protect it (current_mutex).
 *
 * rq is a pointer to a struct used for the ready queue.
 * The head of the queue corresponds to the process
 * that is about to be scheduled onto the CPU, and the tail is for
 * convenience in the enqueue function.
 *
 * Similar to current[], rq is accessed by multiple threads,
 * so a mutex is used to protect it (ready_mutex).
 *
 * The condition variable queue_not_empty is used
 * in conditional waits and signals.
 *
 * The scheduler_algorithm variable and sched_algorithm_t enum help
 * keep track of the scheduler's current scheduling algorithm.
 */
static pcb_t **current;
static queue_t *rq;

static pthread_mutex_t current_mutex;
static pthread_mutex_t queue_mutex;
static pthread_cond_t queue_not_empty;

static sched_algorithm_t scheduler_algorithm;
static unsigned int cpu_count;
static unsigned int age_weight;
static unsigned int time_slice;

/**
 * priority_with_age() is a helper function to calculate the priority of a process
 * taking into consideration the age of the process.
 * 
 * It is determined by the formula:
 * Priority With Age = Priority - (Current Time - Enqueue Time) * Age Weight
 * 
 * @param current_time current time of the simulation
 * @param process process that we need to calculate the priority with age
 * 
 */
extern double priority_with_age(unsigned int current_time, pcb_t *process) {
    return process->priority - (current_time - process->enqueue_time) * age_weight;
}

/**
 * enqueue() is a helper function to add a process to the ready queue.
 *
 * @param queue pointer to the ready queue
 * @param process process that we need to put in the ready queue
 */
void enqueue(queue_t *queue, pcb_t *process)
{
    process->next = NULL;
    process->enqueue_time = get_current_time();

    if (is_empty(queue)) {
        queue->head = process;
        queue->tail = process;
    } else {
        queue->tail->next = process;
        queue->tail = process;
    }
}

/**
 * dequeue() is a helper function to remove a process to the ready queue.
 * 
 * @param queue pointer to the ready queue
 */
pcb_t *dequeue(queue_t *queue)
{
    if (is_empty(queue)) {
        return NULL;
    }

    pcb_t *process = queue->head;
    queue->head = process->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    process->next = NULL;
    return process;
}

/**
 * is_empty() is a helper function that returns whether the ready queue
 * has any processes in it.
 * 
 * @param queue pointer to the ready queue
 * 
 * @return a boolean value that indicates whether the queue is empty or not
 */
bool is_empty(queue_t *queue)
{
    return queue->head == NULL;
}

/**
 * schedule() is the CPU scheduler.
 * 
 * @param cpu_id the target cpu we decide to put our process in
 */
static void schedule(unsigned int cpu_id)
{
    pcb_t *next_process = NULL;

    pthread_mutex_lock(&queue_mutex);

    if (!is_empty(rq)) {
        if (scheduler_algorithm == FCFS) {
            pcb_t *current_p = rq->head;
            pcb_t *prev_p = NULL;
            pcb_t *best_p = rq->head;
            pcb_t *prev_best_p = NULL;
            unsigned int earliest_arrival = best_p->arrival_time;

            while (current_p != NULL) {
                if (current_p->arrival_time < earliest_arrival) {
                    earliest_arrival = current_p->arrival_time;
                    best_p = current_p;
                    prev_best_p = prev_p;
                }
                prev_p = current_p;
                current_p = current_p->next;
            }
            next_process = best_p; 
            if (prev_best_p == NULL) {
                rq->head = next_process->next;
            } else {
                prev_best_p->next = next_process->next;
            }
            if (rq->tail == next_process) {
                rq->tail = prev_best_p;
            }

        } else if (scheduler_algorithm == PA) {
            unsigned int current_time = get_current_time();
            pcb_t *current_p = rq->head;
            pcb_t *prev_p = NULL;
            pcb_t *best_p = rq->head;
            pcb_t *prev_best_p = NULL;
            double best_metric = priority_with_age(current_time, best_p);

            while (current_p != NULL) {
                double current_metric = priority_with_age(current_time, current_p);
                if (current_metric < best_metric) {
                    best_metric = current_metric;
                    best_p = current_p;
                    prev_best_p = prev_p;
                } else if (current_metric == best_metric) {
                    if (current_p->arrival_time < best_p->arrival_time) {
                        best_p = current_p;
                        prev_best_p = prev_p;
                    }
                }
                prev_p = current_p;
                current_p = current_p->next;
            }
            next_process = best_p;
            if (prev_best_p == NULL) {
                rq->head = next_process->next;
            } else {
                prev_best_p->next = next_process->next;
            }
            if (rq->tail == next_process) {
                rq->tail = prev_best_p;
            }

        } else if (scheduler_algorithm == SRTF) {
            pcb_t *current_p = rq->head;
            pcb_t *prev_p = NULL;
            pcb_t *best_p = rq->head;
            pcb_t *prev_best_p = NULL;
            unsigned int best_metric = best_p->total_time_remaining;

            while (current_p != NULL) {
                if (current_p->total_time_remaining < best_metric) {
                    best_metric = current_p->total_time_remaining;
                    best_p = current_p;
                    prev_best_p = prev_p;
                }
                prev_p = current_p;
                current_p = current_p->next;
            }
            next_process = best_p;
            if (prev_best_p == NULL) {
                rq->head = next_process->next;
            } else {
                prev_best_p->next = next_process->next;
            }
            if (rq->tail == next_process) {
                rq->tail = prev_best_p;
            }

        } else {
            next_process = dequeue(rq);
        }

        if (next_process != NULL) {
            next_process->next = NULL;
        }
    }
    
    pthread_mutex_unlock(&queue_mutex);

    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = next_process;
    pthread_mutex_unlock(&current_mutex);

    if (next_process != NULL) {
        next_process->state = PROCESS_RUNNING;
    }

    int timeslice = (scheduler_algorithm == RR) ? time_slice : -1;
    context_switch(cpu_id, next_process, timeslice);
}

/** 
 * idle() is the idle process.  It is called by the simulator when the idle
 * process is scheduled. This function blocks until a process is added
 * to the ready queue.
 *
 * @param cpu_id the cpu that is waiting for process to come in
 */
extern void idle(unsigned int cpu_id)
{
    pthread_mutex_lock(&queue_mutex);
    while (is_empty(rq)) {
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }
    pthread_mutex_unlock(&queue_mutex);

    schedule(cpu_id);

    /*
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.
     */ 
}

/**
 * preempt() is the handler used in Round-robin, Preemptive Priority, and SRTF scheduling.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 * 
 * @param cpu_id the cpu in which we want to preempt process
 */
extern void preempt(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    pcb_t *process = current[cpu_id];
    pthread_mutex_unlock(&current_mutex);

    if (process != NULL) {
        process->state = PROCESS_READY;
        pthread_mutex_lock(&queue_mutex);
        enqueue(rq, process);
        pthread_cond_signal(&queue_not_empty);
        pthread_mutex_unlock(&queue_mutex);
    }

    schedule(cpu_id);
}

/**
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * @param cpu_id the cpu that is yielded by the process
 */
extern void yield(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    pcb_t *process = current[cpu_id];
    pthread_mutex_unlock(&current_mutex);

    if (process != NULL) {
        process->state = PROCESS_WAITING;
    }

    schedule(cpu_id);
}

/**
 * terminate() is the handler called by the simulator when a process completes.
 * 
 * @param cpu_id the cpu we want to terminate
 */
extern void terminate(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    pcb_t* process = current[cpu_id];
    current[cpu_id] = NULL;
    pthread_mutex_unlock(&current_mutex);

    if (process != NULL) {
        process->state = PROCESS_TERMINATED;
    }

    schedule(cpu_id);
}

/**
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes. 
 * This method also handles priority and SRTF preemption.
 * 
 * @param process the process that finishes I/O and is ready to run on CPU
 */
extern void wake_up(pcb_t *process)
{
    process->state = PROCESS_READY;

    pthread_mutex_lock(&queue_mutex);
    enqueue(rq, process);
    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_mutex);

    if (scheduler_algorithm == PA) {
        unsigned int current_time = get_current_time();
        double waking_priority = priority_with_age(current_time, process);
        int target_cpu = -1;
        double highest_running_priority = -DBL_MAX;
        bool idle_cpu_found = false;

        pthread_mutex_lock(&current_mutex);
        for (unsigned int i = 0; i < cpu_count; ++i) {
            if (current[i] == NULL) {
                idle_cpu_found = true;
                break;
            }
        }

        if (!idle_cpu_found) {
            for (unsigned int i = 0; i < cpu_count; ++i) {
                pcb_t *running_process = current[i];
                double running_priority = priority_with_age(current_time, running_process);
                if (running_priority > highest_running_priority) {
                    highest_running_priority = running_priority;
                    target_cpu = i;
                }
            }
        }
        pthread_mutex_unlock(&current_mutex);

        if (!idle_cpu_found && target_cpu != -1 && waking_priority < highest_running_priority) {
            force_preempt(target_cpu);
        }
    } else if (scheduler_algorithm == SRTF) {
        unsigned int waking_remaining_time = process->total_time_remaining;
        int target_cpu = -1;
        unsigned int largest_remaining_time = 0;
        bool idle_cpu_found = false;

        pthread_mutex_lock(&current_mutex);
        for (unsigned int i = 0; i < cpu_count; ++i) {
            if (current[i] == NULL) {
                idle_cpu_found = true;
                break;
            }
        }
        
        if (!idle_cpu_found) {
            for (unsigned int i = 0; i < cpu_count; ++i) {
                pcb_t *running_process = current[i];
                unsigned int running_remaining_time = running_process->total_time_remaining;
                if (running_remaining_time > largest_remaining_time) {
                    largest_remaining_time = running_remaining_time;
                    target_cpu = i;
                }
            }
        }
        pthread_mutex_unlock(&current_mutex);

        if (!idle_cpu_found && target_cpu != -1 && waking_remaining_time < largest_remaining_time) {
            force_preempt(target_cpu);
        }
    }
}

/**
 * main() simply parses command line arguments, then calls start_simulator().
 */
int main(int argc, char *argv[])
{
    scheduler_algorithm = FCFS;
    age_weight = 0;
    time_slice = -1;

    if (argc < 2) {
        fprintf(stderr, "Multithreaded OS Simulator\n"
                        "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p <age weight> | -s ]\n"
                        "    Default : FCFS Scheduler\n"
                        "         -r : Round-Robin Scheduler\n1\n"
                        "         -p : Priority Aging Scheduler\n"
                        "         -s : Shortest Remaining Time First\n");
        return -1;
    }

    /* Parse the command line arguments */
    cpu_count = strtoul(argv[1], NULL, 0);
    if (cpu_count == 0) {
        fprintf(stderr, "Error: Invalid number of CPUs specified.\n");
        return -1;
    }

    if (argc > 2) {
        if (strcmp(argv[2], "-r") == 0) {
            if (argc != 4) {
                 fprintf(stderr, "Error: -r option requires a timeslice value.\n");
                 return -1;
            }
            scheduler_algorithm = RR;
            unsigned int timeslice_ms = strtoul(argv[3], NULL, 0);
            if (timeslice_ms == 0) {
                 fprintf(stderr, "Error: Invalid time slice specified for -r.\n");
                 return -1;
            }
            time_slice = timeslice_ms / 100;
            if (time_slice == 0 && timeslice_ms > 0) {
                time_slice = 1;
            }
        } else if (strcmp(argv[2], "-p") == 0) {
             if (argc != 4) {
                 fprintf(stderr, "Error: -p option requires an age weight value.\n");
                 return -1;
             }
             scheduler_algorithm = PA;
             age_weight = strtoul(argv[3], NULL, 0);
        } else if (strcmp(argv[2], "-s") == 0) {
            if (argc != 3) {
                fprintf(stderr, "Error: -s option does not take any arguments.\n");
                return -1;
            }
             scheduler_algorithm = SRTF;
        } else {
            fprintf(stderr, "Error: Invalid scheduling algorithm option: %s\n", argv[2]);
            return -1;
        }
    }

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t *) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);
    rq = (queue_t *)malloc(sizeof(queue_t));
    assert(rq != NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}

#pragma GCC diagnostic pop

#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <prodcons_bb.h>
#define BUFFER_LENGTH 5
void prodcons_bb(int nargs, char *args[]);
sid32 producer_sem;
sid32 consumer_sem;
sid32 mutex;
int32 read_idx;
int32 write_idx;
int32 array_q[BUFFER_LENGTH];

// xsh$ run prodcons_bb 
// [#of producer processes]
// [#of consumer processes]
// [#of iterations the producer runs]
// [#of iterations the consumer runs]

shellcmd xsh_run(int nargs, char *args[])
{
    // TODO: Error handling of args
    if ((nargs == 1) || (strncmp(args[1], "list", 5) == 0)){
        printf("prodcons_bb\n");
        return 0;
    }

    /* This will go past "run" and pass the function/process name and its
    * arguments.
    */
    args++;
    nargs--;

    if (strncmp(args[0], "prodcons_bb", 11) == 0){
        /* create a process with the function as an entry point. */
        resume(create((void *)prodcons_bb, 4096, 20, "prodcons_bb", 2, nargs, args));
    }
    else{
        printf("Given argument does not match any defined functions.\n");
        printf("Use:\nrun list\nto list defined functions");
        return 1;
    }
    return 0;
}

void prodcons_bb(int nargs, char *args[]){
    // //TODO: declare globally shared array
    // #define BUFFER_LENGTH 5
    // extern int32 array_q[BUFFER_LENGTH];

    // //TODO: declare globally shared semaphores
    // // Producer_sem = len(buffer) [--]
    // // Consumer_sem = 0 [++]
    args++;
    nargs--;

    if(nargs != 4){
        printf("ERROR: Incorrect number of arguments\n");
        return 1;
    }

    int32 producer_count, consumer_count, producer_iterations, consumer_iterations;
    producer_count = atoi(args[0]);
    consumer_count = atoi(args[1]);
    producer_iterations = atoi(args[2]);
    consumer_iterations = atoi(args[3]);

    // declare globally shared read and write indices
    producer_sem = semcreate(BUFFER_LENGTH);
    consumer_sem = semcreate(0);
    mutex = semcreate(1);

    //initialize read and write indices for the queue
    read_idx = 0;
    write_idx = 0;
    
    // TODO: Error handling of args
    // TODO: Design signalling mechanism
    //create and initialize semaphores to necessary values

    //create producer and consumer processes and put them in ready queue
    for (int32 i = 0; i < producer_count; i++)
    {
        char process_name[] = "producer_";
        char process_number[10];
        sprintf(process_number, "%d", i+1);
        strncat(process_name, process_number, 10);
        resume(create(producer_bb, 1024, 20, process_name, 1, producer_iterations));
    }
    for (int32 i = 0; i < consumer_count; i++)
    {
        char process_name[] = "consumer_";
        char process_number[10];
        sprintf(process_number, "%d", i + 1);
        strncat(process_name, process_number, 10);
        resume(create(consumer_bb, 1024, 20, process_name, 1, consumer_iterations));
    }
    return;
}
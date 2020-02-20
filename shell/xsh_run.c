#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <prodcons_bb.h>
#include <limits.h>
#include <ctype.h>
#include <future.h>

void prodcons_bb(int nargs, char *args[]);
void futures_test(int nargs, char *args[]);

int32 isValidArgument(char *number);
sid32 producer_sem;
sid32 consumer_sem;
sid32 mutex;
int32 read_idx;
int32 write_idx;
int32 array_q[BUFFER_LENGTH];

int32 zero;
int32 one;
int32 two;

shellcmd xsh_run(int nargs, char *args[])
{
    // Error handling of args to xsh_run
    if ((nargs == 1) || (strncmp(args[1], "list", 5) == 0)){
        printf("prodcons_bb\n");
        printf("futures_test\n");
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
    else if (strncmp(args[0], "futures_test", 12) == 0) {
        resume(create((void *)futures_test, 4096, 20, "futures_test", 2, nargs, args));
    }
    else{
        printf("Given argument does not match any defined functions.\n");
        printf("Use:\nrun list\nto list defined functions");
        return 1;
    }
    return 0;
}

void prodcons_bb(int nargs, char *args[]){
    args++;
    nargs--;

    if(nargs != 4){
        printf("ERROR: Incorrect number of arguments\n");
        printf("Usage: run prodcons_bb [# of producer processes] [# of consumer processes] [# of iterations the producer runs] [# of iterations the consumer runs]\n");
        return;
    }

    int32 producer_count, consumer_count, producer_iterations, consumer_iterations;
    for (int32 i = 0; i < 4; i++)
    {
        if(!isValidArgument(args[i])){
            printf("ERROR: Invalid arguments\n");
            printf("Argument should be a positive integer from 0 to %d.\n", INT_MAX);
            return;
        }
    }
    
    // convert args to integers once confirmed that they're alright
    producer_count = atoi(args[0]);
    consumer_count = atoi(args[1]);
    producer_iterations = atoi(args[2]);
    consumer_iterations = atoi(args[3]);

    //initialize semaphores to necessary values
    producer_sem = semcreate(BUFFER_LENGTH);
    consumer_sem = semcreate(0);
    mutex = semcreate(1);

    //initialize read and write indices for the queue
    read_idx = 0;
    write_idx = 0;


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

int32 isValidArgument(char *number)
{
    for (int i = 0; number[i] != '\0'; i++)
    {
        if (!isdigit(number[i]))
            return 0;
        // INT_MAX is 10 digits
        if (i > 10)
        {
            return 0;
        }
    }
    return 1;
}


void future_prodcons(int nargs, char *args[]){
    future_t *f_exclusive, *f_shared;
    f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);
    f_shared = future_alloc(FUTURE_SHARED, sizeof(int), 1);

    zero = 0;
    one = 1;
    two = 2;

    kprintf("Test FUTURE_EXCLUSIVE\n");
    // Test FUTURE_EXCLUSIVE
    resume(create(future_cons, 1024, 20, "fcons1", 1, f_exclusive));
    resume(create(future_prod, 1024, 20, "fprod1", 2, f_exclusive, (char *)&one));
    // resume(create(future_prod, 1024, 20, "fprod1", 2, f_exclusive, &a));

    kprintf("Test FUTURE_SHARED\n");
    // Test FUTURE_SHARED
    resume(create(future_cons, 1024, 20, "fcons2", 1, f_shared));
    resume(create(future_cons, 1024, 20, "fcons3", 1, f_shared));
    resume(create(future_cons, 1024, 20, "fcons4", 1, f_shared));
    resume(create(future_cons, 1024, 20, "fcons5", 1, f_shared));
    resume(create(future_prod, 1024, 20, "fprod2", 2, f_shared, (char *)&two));
    return;
}

// int32 fibfut[];

// void future_fibonacci(int nargs, char *args[]){
//     int fib = -1, i;

//     fib = atoi(args[2]);

//     if (fib > -1)
//     {
//         int final_fib;
//         int future_flags = FUTURE_XXXXXXXXX; // TODO - add appropriate future mode here

//         // create the array of future pointers
//         if ((fibfut = (future_t **)getmem(sizeof(future_t *) * (fib + 1))) == (future_t **)SYSERR)
//         {
//             printf("getmem failed\n");
//             return (SYSERR);
//         }

//         // get futures for the future array
//         for (i = 0; i <= fib; i++)
//         {
//             if ((fibfut[i] = future_alloc(future_flags, sizeof(int), 1)) == (future_t *)SYSERR)
//             {
//                 printf("future_alloc failed\n");
//                 return (SYSERR);
//             }
//         }

//         // spawn fib threads and get final value
//         // TODO - you need to add your code here

//         future_get(fibfut[fib], (char *)&final_fib);

//         for (i = 0; i <= fib; i++)
//         {
//             future_free(fibfut[i]);
//         }

//         freemem((char *)fibfut, sizeof(future_t *) * (fib + 1));
//         printf("Nth Fibonacci value for N=%d is %d\n", fib, final_fib);
//         return (OK);
//     }
// }

void futures_test(int nargs, char *args[]){
    args++;
    nargs--;
    if(strncmp("-pc", args[0], 3) == 0){
        resume(create(future_prodcons, 1024, 20, "future_prodcons", 1, NULL));
    }
    return;
}
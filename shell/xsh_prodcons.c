#include <xinu.h>
#include <prodcons.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

int n; //Definition for global variable 'n'
/*Now global variable n will be on Heap so it is accessible all the processes i.e. consume and produce*/
int isValidNumber(char *number);

shellcmd xsh_prodcons(int nargs, char *args[]){
    //Argument verifications and validations
    int count = 2000; //local varible to hold count

    //check args[1] if present assign value to count
    if(nargs > 2){
        printf("Too many arguments.\n");
        printf("Usage: prodcons [integer]\n");
    }
    if(nargs == 2){
        // Error handling of argument
        if (!isValidNumber(args[1])){
            printf("Argument should be a positive integer from 1 to %d.\n", INT_MAX);
            return 1;
        }
        long temp = atol(args[1]);
        if(temp > INT_MAX || temp <= 0){
            printf("Argument should be a positive integer from 1 to %d.\n", INT_MAX);
            return 1;
        }
        count = atoi(args[1]);
    }
    //create the process producer and consumer and put them in ready queue.
    //Look at the definations of function create and resume in the system folder for reference.
    resume(create(producer, 1024, 20, "producer", 1, count));
    resume(create(consumer, 1024, 20, "consumer", 1, count));
    return (0);
}

int isValidNumber(char *number)
{
    for (int i = 0; number[i] != '\0'; i++)
    {
        if (!isdigit(number[i]))
            return 0;
        // INT_MAX is 10 digits
        if(i > 10){
            return 0;
        }
    }
    return 1;
}
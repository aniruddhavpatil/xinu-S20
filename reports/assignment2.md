# Assignment 2 Report

- Does your program output any garbage? If yes, why?
    
    Yes. The xsh shell prompt is displayed while the producer and consumer are printing their outputs. I think it is because of the print statement not being an atomic operatiion. Since it is not atomic, the print statements from the producer and consumer complete in parts.

-  Are all the produced values getting consumed? Check your program for a small count like 20.
    
    No. Not all produced values are getting consumed. Since the print operation is not atomic, multiple updates to the variable 'n' take place before the process prints its value. The following is a sample output:

    ```
    xsh $ prodcons 20
    Produced : 1
    PConsumed : 0
    Consuxsh $ roduced : 2
    Pmed : 0
    Consumed : 0
    roduced : 3
    Produced : Consumed : 0
    Consumed : 04
    Produced : 5
    Produced :
    Consumed : 0
    Consumed : 0
    6
    Produced : 7
    Produced : 8Consumed : 0
    Consumed
    Produced : 9
    Pr : 0
    Consumed :oduced : 10
    Produce 0
    Consumed : 0
    Consumed : 0
    Cd : 11
    Produced : 12
    Produced : 13onsumed : 0
    Consumed : 0
    Consumed : 0
    Co
    Produced : 14
    Produced :nsumed : 0
    Consumed : 0
    Consum 15
    Produced : 16
    Proed : 0
    Consumed duced : 17
    Produced : 18: 0
    Consumed
    Produced : 19
    P: 0
    roduced : 20
    ```


- Write down all the functions in the project.

    1. shellcmd xsh_prodcons(int nargs, char *args[])
     
        Creates the process producer and consumer and puts them in ready queue.
    
    2. int isValidNumber(char *number)
        
        Helper function to validate whether the given argument is a number less than or equal to INT_MAX represented as a character string.
    
    3. void producer(int count)
        
        Iterates from 0 to count, setting the value of the global variable 'n' each time.
    
    4. void consumer(int count)
    
        Reads the value of the global variable 'n' 'count' times and prints it.
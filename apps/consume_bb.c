#include <xinu.h>
#include <prodcons_bb.h>

void consumer_bb(int32 count)
{
    for (int32 i = 0; i < count; i++)
    {
        wait(consumer_sem);
        wait(mutex);
        int32 consumed_value = array_q[read_idx];
        printf("%s \t read: %d \t at: %d\n", proctab[getpid()].prname, consumed_value, read_idx);
        ++read_idx;
        read_idx %= BUFFER_LENGTH;
        signal(mutex);
        signal(producer_sem);
    }
    return;
    
    // Iterate from 0 to count and for each iteration read the next available value from the global array `arr_q`
    // print consumer process name and read value as,
    // name : consumer_1, read : 
}
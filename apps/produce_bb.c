#include <xinu.h>
#include <prodcons_bb.h>

void producer_bb(int count)
{
    for (int32 i = 0; i < count; i++)
    {
        wait(producer_sem);
        // TODO: wait(mutex);
        wait(mutex);
        array_q[write_idx] = i;
        printf("prod_sem: %d name: %s, write: %d, at: %d\n", producer_sem, proctab[getpid()].prname, i, write_idx);
        ++write_idx;
        write_idx %= BUFFER_LENGTH;
        signal(mutex);
        signal(consumer_sem);
    }
    return;
    
    // Iterate from 0 to count and for each iteration add iteration value to the global array `arr_q`,
    // print producer process name and written value as,
    // name : producer_1, write : 8
}

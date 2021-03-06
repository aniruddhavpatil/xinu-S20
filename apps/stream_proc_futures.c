#include <xinu.h>
#include <stream_proc.h>
#include <future.h>
#include "tscdf.h"
#include <stdlib.h>

void stream_consumer_future(int32 id, future_t* fut);
int32 stream_proc_futures(int nargs, char *args[]);
uint pcport;
int32 work_queue_depth, time_window, output_time, num_streams;

int32 stream_proc_futures(int nargs, char *args[])
{
    ulong secs, msecs, time;
    secs = clktime;
    msecs = clkticks;
    // Parse arguments
    char usage[] = "Usage: -s num_streams -w work_queue_depth -t time_window -o output_time\n";

    /* Parse arguments out of flags */
    /* if not even # args, print error and exit */
    if (!(nargs % 2))
    {
        printf("%s", usage);
        return (-1);
    }
    int32 i, num_streams;
    i = nargs - 1;

    // Iterate through args, while populating options
    while (i > 0)
    {
        char *ch = args[i - 1];
        char c = *(++ch);

        switch (c)
        {
        case 's':
            num_streams = atoi(args[i]);
            break;

        case 'w':
            work_queue_depth = atoi(args[i]);
            break;

        case 't':
            time_window = atoi(args[i]);
            break;

        case 'o':
            output_time = atoi(args[i]);
            break;

        default:
            printf("%s", usage);
            return (-1);
        }

        i -= 2;
    }

    if ((pcport = ptcreate(num_streams)) == SYSERR)
    {
        printf("ptcreate failed\n");
        return (-1);
    }

    // Create futures array
    future_t **farray = (future_t **)getmem(sizeof(future_t *) * num_streams);
    for (int32 i = 0; i < num_streams; i++)
    {
        farray[i] = future_alloc(FUTURE_QUEUE, sizeof(de), work_queue_depth);
        // Create consumer processes
        resume(create((void *)stream_consumer_future, 2048, 20, "consumer", 2, i, farray[i]));
    }

    // Parse input header file data and populate work queue
    for (int32 i = 0; i < n_input; i++)
    {

        // Extract stream_id, time_stamp and value from stream_input
        char *a = (char *)stream_input[i];
        int32 stream_id = atoi(a);
        while (*a++ != '\t');
        int32 time_stamp = atoi(a);
        while (*a++ != '\t');
        int32 value = atoi(a);

        // Write time_stamp and value to sreams[stream_id]->queue
        de* produced_element = (de*)getmem(sizeof(de));
        produced_element->time = time_stamp;
        produced_element->value = value;

        future_set(farray[stream_id], (char*)produced_element);
    }

    // Hear back from all consumers
    for (i = 0; i < num_streams; i++)
    {
        uint32 pm;
        pm = ptrecv(pcport);
        printf("process %d exited\n", pm);
    }

    ptdelete(pcport, 0);

    // Parse input header file data and populate work queue
    time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
    printf("time in ms: %u\n", time);

    return 0;
}

void stream_consumer_future(int32 id, future_t* fut)
{
    kprintf("stream_consumer_future id:%d (pid:%d)\n", id, getpid());
    struct tscdf *tptr = tscdf_init(time_window);
    int upt;
    int count = 0;
    char *output;
    int *qarray = (int32 *)getmem(6 * sizeof(int32));

    while (1)
    {

        de* consumed_element;
        // extract consumed_element
        future_get(fut, consumed_element);
        upt = tscdf_update(tptr, consumed_element->time, consumed_element->value);
        // check if consumed_element's time is 0
        if (consumed_element->time == 0) break;

        count++;
        if (upt == SYSERR)
        {
            return SYSERR;
        }

        if (count == output_time)
        {
            // reset count
            count = 0;
            // clear the output string
            output = "";
            // get quartile array from tscdf
            qarray = tscdf_quartiles(tptr);
            // check if qarray is NULL
            if (qarray == NULL)
            {
                kprintf("tscdf_quartiles returned NULL\n");
                continue;
            }
            // pring the qarray values
            sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
            kprintf("%s\n", output);
            // free qarray
            freemem((char *)qarray, (6 * sizeof(int32)));
        }
    }
    tscdf_free(tptr);
    future_free(fut);
    kprintf("process %d exited\n", getpid());
    ptsend(pcport, getpid());
    return;
}
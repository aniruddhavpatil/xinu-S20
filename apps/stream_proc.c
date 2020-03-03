#include <xinu.h>
#include <stream_proc.h>
#include <tscdf_input.h>

void stream_consumer(int32 id, struct stream *str);
int32 stream_proc(int nargs, char *args[]);

int32 stream_proc(int nargs, char *args[])
{
    // Parse arguments
    char usage[] = "Usage: -s num_streams -w work_queue_depth -t time_window -o output_time\n";

    /* Parse arguments out of flags */
    /* if not even # args, print error and exit */
    if (nargs != 4)
    {
        printf("%s", usage);
        return (-1);
    }
    int32 i, num_streams, work_queue_depth, time_window, output_time;
    i = nargs - 1;
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
    
    // Create streams

    // Create consumer processes and initialize streams
    // Use `i` as the stream id.
    for (int32 i = 0; i < num_streams; i++)
    {
        i = i;
    }

    // Parse input header file data and populate work queue

    return 0;
}

void stream_consumer(int32 id, struct stream *str){
    // TODO: Stream Consumer
}
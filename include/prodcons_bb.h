// declare globally shared array
#define BUFFER_LENGTH 5
extern int32 array_q[BUFFER_LENGTH];

// declare globally shared semaphores
extern sid32 producer_sem;
extern sid32 consumer_sem;
extern sid32 mutex;

// declare globally shared read and write indices
extern int32 read_idx;
extern int32 write_idx;

// function prototypes
void consumer_bb(int32 count);
void producer_bb(int32 count);
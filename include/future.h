#include <xinu.h>

#ifndef _FUTURE_H_
#define _FUTURE_H_

typedef enum{
    FUTURE_EMPTY,
    FUTURE_WAITING,
    FUTURE_READY
} future_state_t;

typedef enum{
    FUTURE_EXCLUSIVE,
    FUTURE_SHARED,
    FUTURE_QUEUE
} future_mode_t;

typedef struct future_t{
    char *data;
    uint size;
    future_state_t state;
    future_mode_t mode;
    pid32 pid;
    qid16 set_queue;
    qid16 get_queue;
} future_t;

/* Interface for the Futures system calls */
future_t *future_alloc(future_mode_t mode, uint size, uint nelems);
syscall future_free(future_t *);
syscall future_get(future_t *, char *);
syscall future_set(future_t *, char *);

extern uint future_prod(future_t* fut, char* value);
extern uint future_cons(future_t* fut);
extern int ffib(int n);
extern future_t** fibfut;

extern int32 zero;
extern int32 one;
extern int32 two;

#endif /* _FUTURE_H_ */
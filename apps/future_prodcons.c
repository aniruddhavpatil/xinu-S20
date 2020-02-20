#include <xinu.h>
#include <future.h>

uint future_prod(future_t *fut, char *value){
    // kprintf("Entered future_prod.\n");
    int *nptr = (int *)value;
    kprintf("Produced %d\n", *nptr);
    future_set(fut, value);
    return OK;
}

uint future_cons(future_t *fut){
    // kprintf("Entered future_cons.\n");
    int i, status;
    status = (int)future_get(fut, (char *)&i);
    if (status < 1){
        kprintf("future_get failed\n");
        return -1;
    }
    kprintf("Consumed %d\n", i);

    return OK;
}
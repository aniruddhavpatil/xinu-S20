#include <xinu.h>
#include <future.h>

future_t *future_alloc(future_mode_t mode, uint size, uint nelems);
syscall future_free(future_t *f);
syscall future_get(future_t *f, char *out);
syscall future_set(future_t *f, char *in);

future_t *future_alloc(future_mode_t mode, uint size, uint nelems){
    future_t* fut = (future_t*)getmem(sizeof(future_t));
    if(fut == (future_t*)SYSERR) return SYSERR;
    fut->state = FUTURE_EMPTY;
    fut->mode = mode;
    fut->size = size;
    fut->data = (char*)getmem(size);
    fut->get_queue = newqueue();
    fut->set_queue = newqueue();
    fut->pid = -1;

    return fut;
}

syscall future_set(future_t *f, char *in){
    intmask mask;
    mask = disable();
    // kprintf("Entered future_set\n");
    if(f->state == FUTURE_READY){
        restore(mask);
        return SYSERR;
    }
    // f->data = in;
    memcpy(f->data, in, f->size);
    f->state = FUTURE_READY;
    // if(f->mode=FUTURE_EXCLUSIVE){
    //     kprintf("Resuming consumer.\n");
    //     resume(f->pid);
    //     restore(mask);
    //     return OK;
    // }
    pid32 pid;
    do{
        pid = dequeue(f->get_queue);
        if(pid == EMPTY){
            // f->state = FUTURE_WAITING;
            kprintf("pid was empty\n");
            restore(mask);
            return OK;
        }
        kprintf("Resuming consumer with pid %d\n", pid);
        resume(pid);
    } while(pid != EMPTY);
    restore(mask);
    return OK;
}

syscall future_get(future_t *f, char *out){
    intmask mask;
    mask = disable();
    if(f->mode == FUTURE_EXCLUSIVE){
        if(f->state == FUTURE_EMPTY){
            f->pid = getpid();
            enqueue(getpid(), f->get_queue);
            suspend(getpid());
            if(f->state == FUTURE_EMPTY || f->state == FUTURE_WAITING){
                restore(mask);
                return SYSERR;
            }
            memcpy(out, f->data, f->size);
            f->state = FUTURE_EMPTY;
            restore(mask);
            return OK;
        }
        else{
            restore(mask);
            return SYSERR;
        }
    }
    else if (f->mode == FUTURE_SHARED){
        if (f->state == FUTURE_EMPTY)
        {
            f->pid = getpid();
            enqueue(getpid(), f->get_queue);
            suspend(getpid());
            if (f->state == FUTURE_EMPTY)
            {
                restore(mask);
                return SYSERR;
            }
            memcpy(out, f->data, f->size);
            f->state = FUTURE_WAITING;
            restore(mask);
            return OK;
        }
        else
        {
            restore(mask);
            return SYSERR;
        }
    }
    // if(f->state == FUTURE_EMPTY){
    //     f->pid = getpid();
    //     enqueue(getpid(), f->get_queue);
    //     f->state = FUTURE_WAITING;
    //     kprintf("Enqueued %d\n", getpid());
    //     suspend(getpid());
    //     kprintf("%d woke up \n", getpid());
    //     if (f->mode == FUTURE_EXCLUSIVE)
    //     {
    //         if (f->state == FUTURE_WAITING)
    //         {
    //             restore(mask);
    //             return SYSERR;
    //         }
    //         // out = f->data;
    //         memcpy(out, f->data, f->size);
    //         f->state = FUTURE_EMPTY;
    //         restore(mask);
    //         return OK;
    //     }
    //     else if (f->mode == FUTURE_SHARED)
    //     {
    //         kprintf("FUTURE SHARED\n");
    //         if (f->state == FUTURE_WAITING)
    //         {
    //             enqueue(getpid(), f->get_queue);
    //             suspend(getpid());
    //             kprintf("Woke up f->state is %d.\n", f->state);
    //             if(f->state == FUTURE_READY){
    //                 kprintf("Compying data after waking up.\n");
    //                 memcpy(out, f->data, f->size);
    //                 restore(mask);
    //                 return OK;
    //             }
    //             restore(mask);
    //             return SYSERR;
    //         }
    //         // else if (f->state == FUTURE_READY)
    //         // {
    //         //     // out = f->data;
    //         //     memcpy(out, f->data, f->size);
    //         //     restore(mask);
    //         //     return OK;
    //         // }
    //     }
    //     restore(mask);
    //     return OK;
    // }

    
}

syscall future_free(future_t* f){
    intmask mask;
    mask = disable();
    if(freemem(f->data, f->size) == SYSERR){
        restore(mask);
        return SYSERR;
    }
    if(freemem(f, sizeof(future_t)) == SYSERR){
        restore(mask);
        return SYSERR;
    }
    restore(mask);
    return OK;
}
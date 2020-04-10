#include <xinu.h>
#include <future.h>

future_t *future_alloc(future_mode_t mode, uint size, uint nelems);
syscall future_free(future_t *f);
syscall future_get(future_t *f, char *out);
syscall future_set(future_t *f, char *in);

future_t *future_alloc(future_mode_t mode, uint size, uint nelems){
    future_t* fut = (future_t*)getmem(sizeof(future_t));
    if(fut == (future_t*)SYSERR) return (future_t*)SYSERR;
    fut->state = FUTURE_EMPTY;
    fut->mode = mode;
    fut->size = size;
    fut->data = (char*)getmem(size * nelems);
    // fut->data = ((char *)fut) + sizeof(future_t);
    fut->get_queue = newqueue();
    fut->set_queue = newqueue();
    fut->pid = EMPTY;
    fut->max_elems = nelems;
    fut->head = 0;
    fut->tail = 0;
    fut->count = 0;

    return fut;
}

syscall future_set(future_t *f, char *in){
    intmask mask;
    mask = disable();
    if(f->mode == FUTURE_EXCLUSIVE || f->mode == FUTURE_SHARED){
        if(f->state == FUTURE_READY){
            restore(mask);
            return SYSERR;
        }
        memcpy(f->data, in, f->size);
        f->state = FUTURE_READY;
        pid32 pid;
        do{
            pid = dequeue(f->get_queue);
            if(pid == EMPTY){
                restore(mask);
                return OK;
            }
            resume(pid);
        } while(pid != EMPTY);
    }
    else{
        // kprintf("SET FUTURE_QUEUE mode\n");
        if(f->count >= f->max_elems){
            enqueue(getpid(), f->set_queue);
            // kprintf("Suspending prname: %s, f->count: %d, f->max_elems: %d\n",proctab[getpid()].prname, f->count, f->max_elems);
            suspend(getpid());
            // Do something after waking up
            // char *tailelemptr = f->data + (f->tail * f->size);
            // memcpy(tailelemptr, in, f->size);
            // f->tail++;
            // f->count++;
            // f->tail %= f->max_elems;
        }
        // else{
        char *tailelemptr = f->data + (f->tail * f->size);
        memcpy(tailelemptr, in, f->size);
        f->tail++;
        f->count++;
        f->tail %= f->max_elems;
        pid32 pid = dequeue(f->get_queue);
        // kprintf("Resumed prname: %s, f->count: %d, f->max_elems: %d\n", proctab[getpid()].prname, f->count, f->max_elems);

        if (pid == EMPTY)
        {
            restore(mask);
            return OK;
            }
            // kprintf("SET prname: %s, f->count: %d, f->max_elems: %d\n", proctab[getpid()].prname, f->count, f->max_elems);

            resume(pid);
        // }
    }
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
        if (f->state == FUTURE_EMPTY || f->state == FUTURE_WAITING)
        {
            f->pid = getpid();
            enqueue(getpid(), f->get_queue);
            f->state = FUTURE_WAITING;
            suspend(getpid());
            memcpy(out, f->data, f->size);
            restore(mask);
            return OK;
        }
        else
        {
            memcpy(out, f->data, f->size);
            restore(mask);
            return OK;
        }
    }
    // FUTURE_QUEUE mode
    else {
        // kprintf("GET FUTURE_QUEUE mode\n");
        if(f->count <= 0){
            enqueue(getpid(), f->get_queue);
            // kprintf("Suspending prname: %s, f->count: %d, f->max_elems: %d\n", proctab[getpid()].prname, f->count, f->max_elems);
            suspend(getpid());
            // Do something after waking up
            // char *headelemptr = f->data + (f->head * f->size);
            // memcpy(out, headelemptr, f->size);
            // f->head++;
            // f->count--;
            // f->head %= f->max_elems;
        }
        // else{

        char *headelemptr = f->data + (f->head * f->size);
        memcpy(out, headelemptr, f->size);
        f->head++;
        f->count--;
        f->head %= f->max_elems;
        pid32 pid = dequeue(f->set_queue);
        // kprintf("Resumed prname: %s, f->count: %d, f->max_elems: %d\n", proctab[getpid()].prname, f->count, f->max_elems);

        if (pid == EMPTY)
        {
            restore(mask);
            return OK;
            }
            // kprintf("GET prname: %s, f->count: %d, f->max_elems: %d\n", proctab[getpid()].prname, f->count, f->max_elems);
            resume(pid);
        // }
        restore(mask);
        return OK;
    }
}

syscall future_free(future_t* f){
    intmask mask;
    mask = disable();
    if(freemem(f->data, f->size) == SYSERR){
        restore(mask);
        return SYSERR;
    }
    if(freemem((char*)f, sizeof(future_t)) == SYSERR){
        restore(mask);
        return SYSERR;
    }
    restore(mask);
    return OK;
}
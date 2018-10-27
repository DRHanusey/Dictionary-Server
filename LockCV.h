#include "csapp.h"

#ifndef TRY_STRUCT_H
#define TRY_STRUCT_H

typedef struct {
    int *buf; /* Buffer array */
    
    int capacity; /* Maximum number of slots */
    int occupiedCount; /* #occupied slots */
    
    int front; /* buf[(front+1)%n] is first item */
    int rear; /* buf[rear%n] is last item */

    pthread_mutex_t lock;
    pthread_cond_t empty, full;
} boundedBuffer;


void sbuf_init(boundedBuffer *sp, int capacity) {
    sp->buf = calloc(capacity, sizeof (int));
    sp->capacity = capacity; /* Buffer holds max of n items */
    sp->front = sp->rear = sp->occupiedCount = 0; /* Empty buffer if front == rear */
    sp->lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_init(&sp->empty, NULL);
    pthread_cond_init(&sp->full, NULL);
}

/* Clean up buffer sp */
void sbuf_deinit(boundedBuffer *sp) {
    free(sp->buf);
    pthread_cond_destroy(sp->full);
    pthread_cond_destroy(sp->empty);
    pthread_mutex_destroy(sp->lock);
}

int get(boundedBuffer *sp){
    pthread_mutex_lock(&sp->lock); //acquire lock
    while (sp->occupiedCount == 0){  //while buffer is empty ////////
        pthread_cond_wait(&sp->empty, &sp->lock); // wait for cv 'empty' to be signaled
    }
    
    // critical section 
    int item = sp->buf[sp->front % sp->capacity];  //item to be removed from front
    sp->front++;  //update front of buffer
    sp->occupiedCount--; //update occupied count of buffer
    
    pthread_mutex_unlock(&sp->lock); //release lock
    pthread_cond_signal(&sp->full); //signal threads waiting for cv 'full' 
    
    return item;
}

void put(boundedBuffer *sp, int item){
    pthread_mutex_lock(&sp->lock); //acquire lock
    while (sp->occupiedCount == sp->capacity){  //while buffer is full
        pthread_cond_wait(&sp->full, &sp->lock); // wait for cv 'full' to be signaled
    }
    
    // critical section 
    sp->buf[sp->rear % sp->capacity] = item;  //item to be inserted at rear
    sp->rear++;  //update rear of buffer
    sp->occupiedCount++; //update occupied count of buffer
    
    pthread_mutex_unlock(&sp->lock); //release lock
    pthread_cond_signal(&sp->empty); //signal threads waiting for cv 'empty' 
}

void print_buffer(boundedBuffer *sp){
    printf("front = %d\n", sp->front);
    printf("rear = %d\n", sp->rear);
    printf("occupiedCount = %d\n", sp->occupiedCount);
}


#endif /* TRY_STRUCT_H */


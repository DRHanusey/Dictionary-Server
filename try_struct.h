
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

void sbuf_init(boundedBuffer *sp, int n);
void sbuf_deinit(boundedBuffer *sp);
void put(boundedBuffer *sp, int item);
int get(boundedBuffer *sp);




#endif /* TRY_STRUCT_H */


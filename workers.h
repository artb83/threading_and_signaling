#ifndef SERVER_AESDSOCKET_H
#define SERVER_AESDSOCKET_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdatomic.h>
#include  <pthread.h>
#include "queue.h"

#define BUFFER_SIZE 1024
#define HEAD_MQ messageQueue
#define NWORKERS 2

typedef struct {
    char* messageBuff;
    pthread_t threadId;
    pthread_mutex_t* mutex;
    pthread_cond_t* cond;
    atomic_bool threadComplete;
}thread_data_t;

struct threads_queue_node_t{
    thread_data_t*   thrData;
    TAILQ_ENTRY(threads_queue_node_t) nodes;
};


thread_data_t* allocAndInitThreadData(pthread_mutex_t* mutex, pthread_cond_t* condition, bool bWriter);


#endif //SERVER_AESDSOCKET_H
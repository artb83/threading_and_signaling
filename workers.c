#include "workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

static TAILQ_HEAD(HEAD_MQ, threads_queue_node_t) qHead;

void* readers(void* thd);
void* messengers(void* thd);

thread_data_t* allocAndInitThreadData(pthread_mutex_t* mutex, pthread_cond_t* condition, bool bWriter) {
    thread_data_t* thd = calloc(1, sizeof(*thd));
    if(thd!=NULL) {
        thd->mutex=mutex;
        atomic_store(&thd->threadComplete, false);
        thd->messageBuff = bWriter ? malloc(BUFFER_SIZE*sizeof(*thd->messageBuff)) : NULL;
        thd->cond = condition;
        if (bWriter && thd->messageBuff==NULL) {
            free(thd);
            thd=NULL;
        }
    }
    return  thd;
}

void* sendMessageThrd(void* thrArg) {
    thread_data_t* thd = (thread_data_t*)thrArg;
    if (thd) {
        pthread_mutex_lock(thd->mutex);
        pthread_cond_wait(thd->cond, thd->mutex);
        thd->threadId = pthread_self();
        snprintf(thd->messageBuff, BUFFER_SIZE, "Thread %lu sent message", thd->threadId);
        struct threads_queue_node_t* qnode = thd ? calloc(1, sizeof(*qnode)) : NULL;
        if (qnode) {
            qnode->thrData = thd;
            TAILQ_INSERT_TAIL(&qHead, qnode, nodes);
        }
        pthread_mutex_unlock(thd->mutex);
        atomic_store(&thd->threadComplete, true);
    }
    return thd;
}

void* readMessage(void* thrArg) {
    thread_data_t* thd = (thread_data_t*)thrArg;
    pthread_mutex_lock(thd->mutex);
    pthread_cond_wait(thd->cond, thd->mutex);
    if(!TAILQ_EMPTY(&qHead)) {
        struct threads_queue_node_t* nodep=NULL;
        // struct threads_queue_node_t* tmp=NULL;
        TAILQ_FOREACH(nodep, &qHead, nodes) {
            if (atomic_load(&nodep->thrData->threadComplete))
            printf("Message read - %s from tid-%lu", nodep->thrData->messageBuff, nodep->thrData->threadId);
        }
    }
    pthread_mutex_unlock(thd->mutex);
    atomic_store(&thd->threadComplete, true);
    return thd;
}

void* manager(void* thrArg) {
    bool bRun=true;
    pthread_cond_t* cond = (pthread_cond_t*)thrArg;
    printf("Enter 'a' to start %d worker threads - ", NWORKERS);
    while (bRun) {
        char option = fgetc(stdin);
        if (option=='a') {
            bRun = false;
            printf("Signaing to all\n");
            pthread_cond_signal(cond);
        }
    }
    return NULL;
}

int main(void) {
    pthread_mutex_t messengers_mutex;
    pthread_mutex_init(&messengers_mutex, NULL);

    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    openlog("workers", LOG_PID, LOG_USER); //Open syslog for writing

    TAILQ_INIT(&qHead);

    int nThreads = 0;

    while(nThreads<NWORKERS) {
        thread_data_t* thd=allocAndInitThreadData(&messengers_mutex, &cond, true);
        int rc = pthread_create(&thd->threadId, NULL, sendMessageThrd, thd);
        if (rc != 0) {
            syslog(LOG_ERR, "pthread_create failed");
            free(thd->messageBuff); free(thd);
            continue;
        }
        ++nThreads;
    }

    //Reader thread
    thread_data_t* readerThd=allocAndInitThreadData(&messengers_mutex, &cond, false);
    int rc = pthread_create(&readerThd->threadId, NULL, readMessage, readerThd);
    if (rc != 0) {
        syslog(LOG_ERR, "pthread_create failed");
        free(readerThd);
    }

    //manager thread
    pthread_t managerThreadId;
    rc = pthread_create(&managerThreadId, NULL, manager, &cond);
    if (rc != 0) {
        syslog(LOG_ERR, "pthread_create failed");
    }
    struct threads_queue_node_t* nodep=NULL;
    // struct threads_queue_node_t* tmp=NULL;
    TAILQ_FOREACH(nodep, &qHead, nodes) {
        if (atomic_load(&nodep->thrData->threadComplete)) {
            pthread_join(nodep->thrData->threadId, NULL);
            TAILQ_REMOVE(&qHead, nodep, nodes);
            free(nodep->thrData->messageBuff);
            free(nodep->thrData);
            free(nodep);
        }
    }
    pthread_join(readerThd->threadId, NULL);
    pthread_join(managerThreadId, NULL);
    free(readerThd);
    pthread_mutex_destroy(&messengers_mutex);
    pthread_cond_destroy(&cond);

    closelog(); //Close syslog
    return 0;
}
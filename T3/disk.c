#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "disk.h"
#include "pss.h"

/*****************************************************
 * Agregue aca los tipos, variables globales u otras
 * funciones que necesite
 *****************************************************/

typedef struct{
    int ready;
    pthread_cond_t w;
} Request;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int busy = 0;
int trackGlobal = 0;
PriQueue *upper, *lower;

void iniDisk(void) {
    upper = makePriQueue();
    lower = makePriQueue();
}

void cleanDisk(void) {
    destroyPriQueue(lower);
    destroyPriQueue(upper);
}

void requestDisk(int track) {
    pthread_mutex_lock(&m);
    if(!busy){
        busy = 1;
        trackGlobal = track;
    }
    else{
        Request req = {0, PTHREAD_COND_INITIALIZER};
        if(track < trackGlobal){
            priPut(lower, &req , track);
        }
        else{
            priPut(upper, &req , track);
        }
        while(!req.ready){
            pthread_cond_wait(&req.w, &m);
        }
    }
    pthread_mutex_unlock(&m);
}
void releaseDisk() {
    pthread_mutex_lock(&m);
    if(emptyPriQueue(upper)){
        if(emptyPriQueue(lower)){
            busy = 0;
        }
        else{
            Request *req = priGet(lower);
            req->ready = 1;
            pthread_cond_signal(&req->w);
        }
    }
    else{
        trackGlobal = priBest(upper);
        Request *req = priGet(upper);
        req->ready = 1;
        pthread_cond_signal(&req->w);
    }
    pthread_mutex_unlock(&m);
}


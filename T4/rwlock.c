#define _XOPEN_SOURCE 500

#include "nthread-impl.h"
#include "rwlock.h"

struct rwlock {
  NthQueue *readersQueue;
  NthQueue *writersQueue;
  int readers;
  int writers;
};

nRWLock *nMakeRWLock() {
  nRWLock *rwl = (nRWLock *)malloc(sizeof(nRWLock));
  rwl->readers = 0;
  rwl->writers = 0;
  rwl->readersQueue = nth_makeQueue();
  rwl->writersQueue = nth_makeQueue();
  return rwl;
}

void nDestroyRWLock(nRWLock *rwl) {
  nth_destroyQueue(rwl->readersQueue);
  nth_destroyQueue(rwl->writersQueue);
  free(rwl);
}
  

int nEnterRead(nRWLock *rwl, int timeout) {
  START_CRITICAL

  if (rwl->writers == 0 && nth_emptyQueue(rwl->writersQueue)) {
    rwl->readers++;
  }
  else {
    nth_putBack(rwl->readersQueue, nSelf());
    suspend(WAIT_RWLOCK);
    schedule();
  }
  END_CRITICAL
  return 1;
}

int nEnterWrite(nRWLock *rwl, int timeout) {
  START_CRITICAL

  if (rwl->readers == 0 && rwl->writers == 0) {
    rwl->writers++;
  }
  else {
    nth_putBack(rwl->writersQueue, nSelf());
    suspend(WAIT_RWLOCK);
    schedule();
  }
  END_CRITICAL
  return 1;
  
}

void nExitRead(nRWLock *rwl) {
  START_CRITICAL
  rwl->readers--;
  if (rwl->readers == 0 && !nth_emptyQueue(rwl->writersQueue)) {
    nThread w = nth_getFront(rwl->writersQueue);
    rwl->writers++;
    setReady(w);  
    schedule();
  }
  
  END_CRITICAL
}

void nExitWrite(nRWLock *rwl) {
  START_CRITICAL
  rwl->writers--;
  if (!nth_emptyQueue(rwl->readersQueue)){
    while(!nth_emptyQueue(rwl->readersQueue)){
      nThread r = nth_getFront(rwl->readersQueue);
      rwl->readers++;
      setReady(r);
      schedule();
    }
  }
  else if (!nth_emptyQueue(rwl->writersQueue)) {
    nThread w = nth_getFront(rwl->writersQueue);
    rwl->writers++;
    setReady(w);
    schedule();
  }
  END_CRITICAL
}

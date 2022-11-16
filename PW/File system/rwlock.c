#include "rwlock.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "err.h"
#include "Tree.h"

void init(struct readwrite *rw) {
  
  if (pthread_mutex_init(&rw->lock, 0)!= 0)
    syserr ("mutex init failed");
  if (pthread_cond_init(&rw->readers, 0)!= 0)
    syserr ("cond init 1 failed");
  if (pthread_cond_init(&rw->writers, 0)!= 0)
    syserr ("cond init 2 failed");
  rw->rcount = 0;
  rw->wcount = 0;
  rw->rwait = 0;
  rw->wwait = 0;
  rw->howmany = 0;
}

void destroy(struct readwrite *rw) {

  if (pthread_cond_destroy (&rw->writers) != 0)
    syserr ("cond destroy 1 failed");
  if (pthread_cond_destroy (&rw->readers) != 0)
    syserr ("cond destroy 2 failed");
  if (pthread_mutex_destroy (&rw->lock) != 0)
    syserr ("mutex destroy failed");
}

void reader_lock(struct readwrite *rw)
{
    if (pthread_mutex_lock(&rw->lock) != 0)
      syserr ("lock failed");
    rw->rwait++;
    while (rw->wwait + rw->wcount > 0 && rw->howmany == 0)
      if (pthread_cond_wait(&rw->readers, &rw->lock) != 0)
        syserr ("cond wait failed");
    if (rw->howmany > 0)
      rw->howmany--;
    rw->rwait--;
    rw->rcount++;
    if ( pthread_mutex_unlock(&rw->lock) != 0)
      syserr ("unlock failed");
}

void reader_unlock(struct readwrite* rw) {
    if (pthread_mutex_lock(&rw->lock) != 0)
      syserr ("lock failed");
    rw->rcount--;
    if(rw->rcount == 0)
      if (pthread_cond_signal(&rw->writers) != 0)
        syserr ("cond wait failed");
    if (pthread_mutex_unlock(&rw->lock) != 0)
      syserr ("unlock failed");
  
  return;
}

void writer_lock(struct readwrite* rw) {  
    if (pthread_mutex_lock(&rw->lock) != 0)
      syserr ("lock failed");
    rw->wwait++;
    while (rw->rcount + rw->wcount > 0)
      if (pthread_cond_wait(&rw->writers, &rw->lock) != 0)
        syserr ("cond wait failed");
    rw->wwait--;
    rw->wcount++;
    if (pthread_mutex_unlock(&rw->lock) != 0)
      syserr ("unlock failed");
}

void writer_unlock(struct readwrite* rw) {
    if (pthread_mutex_lock(&rw->lock) != 0)
      syserr ("lock failed");
    rw->wcount--;
    if (rw->rwait == 0) {
      if (pthread_cond_signal(&rw->writers) != 0)
        syserr ("signal failed");
    }
    rw->howmany = rw->rwait;
    for (int i = rw->rwait; i > 0; i--) {
      if (pthread_cond_signal(&rw->readers) != 0)
        syserr ("signal failed");
    }
    
    if (pthread_mutex_unlock(&rw->lock) != 0)
      syserr ("unlock failed");
  
  return;
}
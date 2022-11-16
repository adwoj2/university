#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "err.h"
#include "Tree.h"

struct readwrite {
  pthread_mutex_t lock;         
  pthread_cond_t readers;      
  pthread_cond_t writers;    
  int rcount, wcount, rwait, wwait;
  int change;
  int howmany;
};

void init(struct readwrite *rw);

void destroy(struct readwrite *rw);

void reader_lock(struct readwrite *rw);

void reader_unlock(struct readwrite* rw);

void writer_lock(struct readwrite* rw);

void writer_unlock(struct readwrite* rw);

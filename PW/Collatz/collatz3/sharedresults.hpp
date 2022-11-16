#ifndef SHAREDRESULTS_HPP
#define SHAREDRESULTS_HPP
#include <iostream>
#include <map>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <semaphore.h>

#include "err.h"

struct readwrite {
  pthread_mutex_t lock;         
  pthread_cond_t readers;      
  pthread_cond_t writers;    
  int rcount, wcount, rwait, wwait;
  int change;
  int howmany;
};

static void init(struct readwrite *rw) {
  
  pthread_mutex_init(&rw->lock, 0);
  pthread_cond_init(&rw->readers, 0);
  pthread_cond_init(&rw->writers, 0);
  rw->rcount = 0;
  rw->wcount = 0;
  rw->rwait = 0;
  rw->wwait = 0;
  rw->howmany = 0;
}

static void destroy(struct readwrite *rw) {

  pthread_cond_destroy (&rw->writers);
  pthread_cond_destroy (&rw->readers);
  pthread_mutex_destroy (&rw->lock);
}

static void reader_lock(struct readwrite *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->rwait++;
    while (rw->wwait + rw->wcount > 0 && rw->howmany == 0)
        pthread_cond_wait(&rw->readers, &rw->lock);
    if (rw->howmany > 0)
      rw->howmany--;
    rw->rwait--;
    rw->rcount++;
    pthread_mutex_unlock(&rw->lock);
}

static void reader_unlock(struct readwrite* rw) {
    pthread_mutex_lock(&rw->lock);
    rw->rcount--;
    if(rw->rcount == 0)
        pthread_cond_signal(&rw->writers);

    pthread_mutex_unlock(&rw->lock);
  
  return;
}

static void writer_lock(struct readwrite* rw) {  
    pthread_mutex_lock(&rw->lock);
    rw->wwait++;
    while (rw->rcount + rw->wcount > 0)
        pthread_cond_wait(&rw->writers, &rw->lock);
    rw->wwait--;
    rw->wcount++;
    pthread_mutex_unlock(&rw->lock);
}

static void writer_unlock(struct readwrite* rw) {
    pthread_mutex_lock(&rw->lock);
    rw->wcount--;
    if (rw->rwait == 0) {
        pthread_cond_signal(&rw->writers);
    }
    rw->howmany = rw->rwait;
    for (int i = rw->rwait; i > 0; i--) {
        pthread_cond_signal(&rw->readers);
    }
    
    pthread_mutex_unlock(&rw->lock);
  
  return;
}

//klasa łącząca std::promise oraz std::future
class PromiseFuture {
public:
    PromiseFuture() {
        this->f = p.get_future();
    }
    void setResult(uint64_t val) {
        p.set_value(val);
    }

    uint64_t getResult() {
        return f.get();

    }
private:
    std::promise<uint64_t> p;
    std::shared_future<uint64_t> f;
};

class SharedResults
{
public:
    SharedResults() {
        init(&rw);
        addValue(1);
        setValue(1, 0);//dodanie wartości kończących procedurę Collatza
    }
    bool checkAndAddValue(InfInt number, std::vector<InfInt>* calculated) {
        writer_lock(&rw);
        int howmany = results.count(number);
        if (howmany == 0) {
            addValue(number);
            calculated->push_back(number);
        }
        writer_unlock(&rw);
        return howmany != 0;
    }

    void addValue(InfInt number) {
        PromiseFuture pf;
        pf = PromiseFuture();
        results.insert(std::make_pair(number, std::move(pf)));
    }

    void setValue(InfInt number, uint64_t value) {
        writer_lock(&rw);
        results[number].setResult(value);
        writer_unlock(&rw);
    }

    uint64_t getValue(InfInt number) {
        reader_lock(&rw);
        auto record = results.find(number); 
        reader_unlock(&rw);
        return record->second.getResult();
    }

private:
    std::map<InfInt, PromiseFuture> results;
    struct readwrite rw;
};

#endif
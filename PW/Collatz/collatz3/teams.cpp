#include <utility>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <future>
#include <sys/mman.h>
#include <sys/stat.h>  
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

#include "teams.hpp"
#include "contest.hpp"
#include "collatz.hpp"
#include "collatz_shared.hpp"

#define FAILED_TRY false

class Semaphore {
    public:
        Semaphore(int max) {
            this->count = max;
        }
    
    void P() {
        std::unique_lock<std::mutex> lock(m);
        while (count == 0)
            cond.wait(lock);
        count--;
    }
    void V() {
        std::unique_lock<std::mutex> lock(m);
        count++;
        cond.notify_one();

    }
    private:
        int count;
        std::mutex m;
        std::condition_variable cond;
};

//funkcje pomocnicze zapisujące wynik do odpowiedniej struktury
void newThreadsSaveResult (InfInt x, ContestResult * result, uint64_t idx, Semaphore* sem) {
    uint64_t r = calcCollatz(x);
    (*result)[idx] = r;
    (*sem).V();
}

void newThreadsShareResult (InfInt x, ContestResult * result, uint64_t idx, Semaphore* sem, std::shared_ptr<SharedResults> shared) {
    uint64_t r = calcCollatzShared(x, shared);
    (*result)[idx] = r;
    (*sem).V();
}

ContestResult TeamNewThreads::runContestImpl(ContestInput const & contestInput)
{
    ContestResult r;
    uint32_t size = getSize();

    uint64_t idx = 0;
    r.resize(contestInput.size());
    Semaphore sem(size);
    if (this->getSharedResults()) {
        std::shared_ptr<SharedResults> shared = this->getSharedResults();
        for (InfInt const & singleInput : contestInput) {
            sem.P();
            std::thread th = createThread(newThreadsShareResult, singleInput, &r, idx, &sem, shared);
            th.detach();
            idx++;
        }
        for (int i = 0; i < size; i++) {
            sem.P();
        }
        for (int i = 0; i < size; i++)
            sem.V();
        
        return r;
    } else {
        for (InfInt const & singleInput : contestInput) {
            sem.P();
            std::thread th = createThread(newThreadsSaveResult, singleInput, &r, idx, &sem);
            th.detach();
            idx++;
        }
        for (int i = 0; i < size; i++)
            sem.P();
        for (int i = 0; i < size; i++)
            sem.V();
        return r;
    }
}

//funkcje pomocnicze zapisujące wynik do odpowiedniej struktury
void constThreadsSaveResult (ContestInput const & contestInput, ContestResult* result, uint64_t idx, uint32_t threads, uint64_t elements) {
    for (int i = idx; i < elements; i += threads) {
        InfInt x = contestInput[i];
        uint64_t r = calcCollatz(x);
        (*result)[i] = r;
    }
}

void constThreadsShareResult (ContestInput const & contestInput, ContestResult* result, uint64_t idx, uint32_t threads, uint64_t elements, std::shared_ptr<SharedResults> shared) {
    for (int i = idx; i < elements; i += threads) {
        InfInt x = contestInput[i];
        uint64_t r = calcCollatzShared(x, shared);
        (*result)[i] = r;
    }
}

ContestResult TeamConstThreads::runContestImpl(ContestInput const & contestInput)
{
    ContestResult r;
    uint32_t size = getSize();
    r.resize(contestInput.size());
    uint64_t how_many_elements = contestInput.size();
    std::vector<std::thread> threads;
    threads.resize(size);
    if (this->getSharedResults()) {
        std::shared_ptr<SharedResults> shared = this->getSharedResults();
        for (int i = 0; i < size; i++) {
            threads[i] = createThread(constThreadsShareResult, contestInput, &r, i, size, how_many_elements, shared);
        }
    } else {
        for (int i = 0; i < size; i++) {
            threads[i] = createThread(constThreadsSaveResult, contestInput, &r, i, size, how_many_elements);
        }
    }

    for (int i = 0; i < size; i++)
        threads[i].join();

    return r;
}

//funkcje pomocnicze zapisujące wynik do odpowiedniej struktury
void teamPoolSaveResult (InfInt x, ContestResult * result, uint64_t idx) {
    uint64_t r = calcCollatz(x);
    (*result)[idx] = r;
}

void teamPoolShareResult (InfInt x, ContestResult * result, uint64_t idx, std::shared_ptr<SharedResults> shared) {
    uint64_t r = calcCollatzShared(x, shared);
    (*result)[idx] = r;
}

ContestResult TeamPool::runContest(ContestInput const & contestInput)
{
    ContestResult r;
    uint32_t size = getSize();
    r.resize(contestInput.size());
    cxxpool::thread_pool pool{size};
    uint64_t idx = 0;
    if (this->getSharedResults()) {
        std::shared_ptr<SharedResults> shared = this->getSharedResults();
        for(InfInt const & singleInput : contestInput) {
            auto fut = pool.push(teamPoolShareResult, singleInput, &r, idx, shared);
            idx++;
        }   
    } else {
        for(InfInt const & singleInput : contestInput) {
            auto fut = pool.push(teamPoolSaveResult, singleInput, &r, idx);
            idx++;
        }   
    }
    return r;
}

uint64_t calcCollatzSharedForProcesses(InfInt n, std::shared_ptr<SharedResults> sharedresults, PromiseFuture *partial_results, bool *if_calculated, sem_t* sem, sem_t* sem2) {
    uint64_t count = 0;
    /*
    assert(n > 0);
    std::vector<int> calculated;
    
    while (true)
    {  
        if (n < LIMIT) {
            sem_wait(sem);
            if (if_calculated[n.toInt()]) {
                
                sem_post(sem);
                sem_wait(sem2);
                std::cout<<"as"<<std::endl;
                count += partial_results[n.toInt()].getResult();
                std::cout<<"af"<<std::endl;
                sem_post(sem2);
                break;
            } else { 
                std::cout<<"yyp" <<std::endl;
                PromiseFuture pf;
                pf = PromiseFuture();
                partial_results[n.toInt()] = std::move(pf);
                if_calculated[n.toInt()] = true;
                calculated.push_back(n.toInt());
                sem_post(sem);
            }
        }
        ++count;
        if (n % 2 == 1)
        {
            n *= 3;
            n += 1;
        }
        else
        {
            n /= 2;
        }            
    }

    std::vector<int>::iterator it;
    uint64_t counter = count;
    sem_wait(sem);
    for (it = calculated.begin(); it < calculated.end(); it++) {
        std::cout<<"it" << *it << "count" << counter<< std::endl;

        partial_results[*it].setResult(counter);
        counter--;
    }

    sem_post(sem);*/
    return count;
    }

ContestResult TeamNewProcesses::runContest(ContestInput const & contestInput)
{
    ContestResult r;
    uint32_t size = getSize();
    uint64_t idx = 0;
    uint64_t *mapped_mem;
    int fd_memory = -1; 
    int flags, prot;

    prot = PROT_READ | PROT_WRITE;
    flags = MAP_SHARED | MAP_ANONYMOUS;

    mapped_mem = (uint64_t *) mmap(NULL, sizeof(uint64_t) * contestInput.size(), prot, flags, fd_memory, 0);
    if (this->getSharedResults() && FAILED_TRY) {
        void* mapped_mem_all; 
        sem_t *sem;
        sem_t *sem2;

        PromiseFuture* partial_results;
        bool* if_calculated;
        std::shared_ptr<SharedResults> shared = this->getSharedResults();
        mapped_mem_all = mmap(NULL, 2 * sizeof(sem_t) + sizeof(bool) * LIMIT, prot, flags, fd_memory, 0);

        if_calculated = (bool*) (mapped_mem_all + 2 * sizeof(sem_t));
        sem = (sem_t*) mapped_mem_all;
        sem2 = (sem_t*) mapped_mem_all + sizeof(sem_t);
        sem_init(sem, 1, 1);
        sem_init(sem2, 1, 1);

        partial_results = (PromiseFuture*) mmap(NULL, sizeof( PromiseFuture) * LIMIT, prot, flags, fd_memory, 0);
        if_calculated[1] = true;
        partial_results[1] = PromiseFuture();
        partial_results[1].setResult(0);
        for (InfInt const & singleInput : contestInput) {
            if (idx >= size)
                wait(0);

            switch (fork()) {                     
                case -1: 
                    break;
                case 0:    
                {
                    mapped_mem[idx] = calcCollatzSharedForProcesses(singleInput, shared, partial_results, if_calculated, sem, sem2);
                    exit(0);
                    break;
                }
                default:                            
                    break;
            }
            idx++;
        }
        munmap(partial_results, contestInput.size());
        sem_destroy(sem);
    } else {
        for (InfInt const & singleInput : contestInput) {
            if (idx >= size)
                wait(0);

            switch (fork()) {                     
                case -1: 
                    break;
                case 0:                 
                    mapped_mem[idx] = calcCollatz(singleInput);
                    exit(0);
                    break;
                default:                            
                    break;
            }
            idx++;
        }
    }

    r.resize(contestInput.size());
    for (int i = 0; i < size; i++)
        wait(0);
    for (int i = 0; i < contestInput.size(); i++) 
        r[i] = mapped_mem[i];
    

    munmap(mapped_mem, contestInput.size());
    return r;
}

ContestResult TeamConstProcesses::runContest(ContestInput const & contestInput)
{
 ContestResult r;
    uint32_t size = getSize();

    uint64_t idx = 0;
    int how_many = contestInput.size();
    void *mapped_mem_all;
    uint64_t *mapped_mem;

    int fd_memory = -1; 
    int flags, prot;
    prot = PROT_READ | PROT_WRITE;
    flags = MAP_SHARED | MAP_ANONYMOUS;

    mapped_mem = (uint64_t *) mmap(NULL, sizeof(uint64_t) * contestInput.size(), prot, flags, fd_memory, 0);

    for (int id = 0; id < size; id++) {

        switch (fork()) {                     
            case -1: 
                break;
            case 0:  
                uint64_t singleResult; 
                for (int idx = id; idx < how_many; idx += size) {
                    singleResult = calcCollatz(contestInput[idx]);
                    mapped_mem[idx] = singleResult;
                }
                munmap(mapped_mem, contestInput.size()); 
                exit(0);
                break;
            default:                            
                break;
        }
    }
    r.resize(contestInput.size());
    for (int i = 0; i < size; i++)
        wait(0);
    for (int i = 0; i < contestInput.size(); i++) 
        r[i] = mapped_mem[i];
    
    munmap(mapped_mem,  contestInput.size());
    return r;
}

//funkcje pomocnicze zapisujące wynik do odpowiedniej struktury
void asyncSaveResult (InfInt x, ContestResult * result, uint64_t idx) {
    uint64_t v = calcCollatz(x);
    (*result)[idx] = v;
}

void asyncShareResult (InfInt x, ContestResult * result, uint64_t idx, std::shared_ptr<SharedResults> shared) {
    uint64_t v = calcCollatzShared(x, shared);
    (*result)[idx] = v;
}

ContestResult TeamAsync::runContest(ContestInput const & contestInput)
{
    ContestResult r;
    uint32_t size = getSize();
    r.resize(contestInput.size());
    cxxpool::thread_pool pool{size};
    uint64_t idx = 0;
    if (this->getSharedResults()) {
        std::shared_ptr<SharedResults> shared = this->getSharedResults();
        for(InfInt const & singleInput : contestInput) {
            std::future<void> fut = std::async(std::launch::async, asyncShareResult, singleInput, &r, idx, shared);
            idx++;
        }
    } else {
        for(InfInt const & singleInput : contestInput) {
            std::future<void> fut = std::async(std::launch::async, asyncSaveResult, singleInput, &r, idx);
            idx++;
        }
    }
    return r;
}

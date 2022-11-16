#ifndef COLLATZSHARED_HPP
#define COLLATZSHARED_HPP

#include <assert.h>
#include <vector>
#include "sharedresults.hpp"
#define LIMIT 1000000 //stała do drużyn processesX
inline uint64_t calcCollatzShared(InfInt n, std::shared_ptr<SharedResults> sharedresults)
{
    uint64_t count = 0;
    assert(n > 0);
    std::vector<InfInt> calculated;
    while (true)
    {
        if (sharedresults->checkAndAddValue(n, &calculated)) {
            count += sharedresults->getValue(n);
            break;
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
    std::vector<InfInt>::iterator it;
    uint64_t counter = count;
    for (it = calculated.begin(); it < calculated.end(); it++) {
        sharedresults->setValue(*it, counter);
        counter--;
    }
    return count;
}
#endif
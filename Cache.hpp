
#ifndef __Cache_hpp__
#define __Cache_hpp__

#include <cstdlib>
#include <vector>

#include <errno.h>
#include <pthread.h>

#include "CacheItem.hpp"

using namespace std;

class Cache {
    private:
        int               bytesUsed;
        pthread_mutex_t   lock;
        vector<CacheItem*> cache;
        
        int search(const string& url);
    
    public:
        int maxSize;
        
        Cache();
        
        void       insert(CacheItem* item);
        CacheItem* access(const string& url);
};

#endif


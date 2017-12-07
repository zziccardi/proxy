
#include "Cache.hpp"

Cache::Cache() {
    bytesUsed = 0;
    
    int r = pthread_mutex_init(&lock, NULL);
    
    if (r != 0) {
        errno = r;
        perror("pthread_mutex_init() failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * Insert a CacheItem into the cache. If the cache doesn't have enough room to insert the item, the least recently used (LRU) replacement policy is used to remove one or more items.
 * @param item - the item to insert
 */
void Cache::insert(CacheItem* item) {
    pthread_mutex_lock(&lock);
    
    int index = search(item->url);
    
    // If the item isn't already in the cache, add it
    if (index == -1) {
        // If the response exceeds the maximum cache size
        if (item->responseSize > maxSize) {
            cerr << endl << "ERROR: Response for the following URL exceeds maximum cache size (" << maxSize << "): " << item->url << endl << endl;
            exit(EXIT_FAILURE);
        }
        
        // If there isn't room to add the item without removing other items, keep removing the least
        // recently used item until there's enough room
        while (item->responseSize > maxSize - bytesUsed) {
            CacheItem* lastItem = cache.back();
            
            bytesUsed -= lastItem->responseSize;
            
            cache.erase(cache.end() - 1);
            
            delete lastItem;
        }
        
        // Insert the item at the front so it becomes the new most recently used item
        cache.insert(cache.begin(), item);
        
        bytesUsed += item->responseSize;
    }
    // It's already cached, but it's not at index 0, which is where the most recently used element is
    // kept, so move it there
    else if (index != 0) {
        cache.erase(cache.begin() + index);
        
        cache.insert(cache.begin(), item);
    }
    
    pthread_mutex_unlock(&lock);
}

/**
 * If an item is in the cache, return a pointer to it and make it the most recently used item.
 * @param  url  - the URL to search for
 * @return item - a pointer to the CacheItem if found, otherwise nullptr
 */
CacheItem* Cache::access(const string& url) {
    CacheItem* item = nullptr;
    
    pthread_mutex_lock(&lock);
    
    int index = search(url);
    
    // If the item is cached, get a pointer to it
    if (index != -1) {
        item = cache[index];
        
        // If the item is not at index 0, move it there to make it the most recently used item
        if (index != 0) {
            cache.erase(cache.begin() + index);
            
            cache.insert(cache.begin(), item);
        }
    }
    
    pthread_mutex_unlock(&lock);
    
    return item;
}

/**
 * Search the vector for a CacheItem with the provided URL.
 * NOTE: This does not lock!
 * @param  url   - the URL to search for
 * @return index - the index of the CacheItem, or -1 if none is found
 * @private
 */
int Cache::search(const string& url) {
    int index = -1;
    
    for (int i = 0; i < cache.size(); i++) {
        if (cache[i]->url == url) {
            index = i;
            break;
        }
    }
    
    return index;
}


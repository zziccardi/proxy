
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
void Cache::insert(const CacheItem& item) {
    pthread_mutex_lock(&lock);
    
    int index = search(item.url);
    
    // If the item isn't already in the cache, add it
    if (index == -1) {
        // If the response exceeds the maximum cache size
        if (item.responseSize > maxSize) {
            cerr << "Response for " << item.url << " exceeds maximum cache size (" << maxSize << ")" << endl;
            exit(EXIT_FAILURE);
        }
        // If there's room to add the item without removing any others
        else if (item.responseSize <= maxSize - bytesUsed) {
            // Insert the item at the front so it becomes the new most recently used item
            cache.insert(cache.begin(), item);
            
            bytesUsed += item.responseSize;
        }
        // Keep removing the least recently used item until there's enough room for the new one
        else {
            // While there's still not enough room
            while (item.responseSize > maxSize - bytesUsed) {
                int itemSize = cache.back().responseSize;
                
                cache.erase(cache.end() - 1);
                
                bytesUsed -= itemSize;
            }
            
            // Insert the item at the front so it becomes the new most recently used item
            cache.insert(cache.begin(), item);
            
            bytesUsed += item.responseSize;
        }
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
 * If an item is in the cache, get its response text and make it the most recently used item.
 * @param url - the URL to search for
 * @return response - the response text or the empty string if the item is not cached
 */
string Cache::access(const string& url) {
    string response = "";
    
    pthread_mutex_lock(&lock);
    
    int index = search(url);
    
    // If the item is cached, get the response and move the item to index 0
    if (index != -1) {
        CacheItem item(cache[index]);
        
        response = item.response;
        
        cache.erase(cache.begin() + index);
        
        cache.insert(cache.begin(), item);
    }
    
    pthread_mutex_unlock(&lock);
    
    return response;
}

/**
 * Search the vector for a CacheItem with the provided URL
 * NOTE: This does not lock!
 * @param url - the URL to search for
 * @return index of the CacheItem, or -1 if none is found
 * @private
 */
int Cache::search(const string& url) {
    int index = -1;
    
    for (int i = 0; i < cache.size(); i++) {
        if (cache[i].url == url) {
            index = i;
            break;
        }
    }
    
    return index;
}


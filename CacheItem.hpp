
#ifndef __CacheItem_hpp__
#define __CacheItem_hpp__

#include <iostream>
#include <string>

using namespace std;

class CacheItem {
    public:
        string url;
        string response;
        int    responseSize;  // size of the entire response in bytes
        int    contentLength; // as specified by the response header
        
        CacheItem(const string url, const string response, const int responseSize);
};

#endif


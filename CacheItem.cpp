
#include "CacheItem.hpp"

CacheItem::CacheItem(const string url, const string response, const int responseSize) {
    this->url          = url;
    this->response     = response;
    this->responseSize = responseSize;
    
    string substring = response.substr(response.find("Content-Length") + 16);
    
    substring = substring.substr(0, substring.find("\r"));
    
    this->contentLength = stoi(substring);
    
    //cout << "Content length: " << contentLength << endl;
}


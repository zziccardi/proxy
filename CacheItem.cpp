
#include "CacheItem.hpp"

CacheItem::CacheItem(const string& url, const string& response) {
    this->url          = url;
    this->response     = response;
    this->responseSize = response.size();
    
    string substring = response.substr(response.find("Content-Length") + 16);
    
    substring = substring.substr(0, substring.find("\r"));
    
    this->contentLength = stoi(substring);
    
    //cout << "Content length: " << contentLength << endl;
}

CacheItem::CacheItem(const CacheItem& item) {
    this->url           = item.url;
    this->response      = item.response;
    this->responseSize  = item.responseSize;
    this->contentLength = item.contentLength;
}



#ifndef __proxy_hpp__
#define __proxy_hpp__

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Cache.hpp"
#include "CacheItem.hpp"

using namespace std;

using Clock = chrono::high_resolution_clock;

struct ClientInfo {
    int               socket;
    string            ipAddress;
    Clock::time_point startTime;
};

string getProxyHostName();
int    getProxyPort(const int& socket, const struct sockaddr_in& sa);
void*  requestHandler(void* ci);
void   talkToServer(const string& request, const string& url);
int    connectToServer(const string& hostName, const string& port);

#endif


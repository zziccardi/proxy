
#ifndef __proxy_hpp__
#define __proxy_hpp__

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

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

string getProxyHostName();
int    getProxyPort(const int& socket, const struct sockaddr_in& sa);
void*  requestHandler(void* fd);
string talkToServer(const string& request, const string& url);
int    connectToServer(const string& hostName);

#endif


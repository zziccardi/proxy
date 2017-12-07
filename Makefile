
all: link

proxy: proxy.cpp
	g++ -std=c++11 -pthread -g -c proxy.cpp -o proxy.o

Cache: Cache.cpp
	g++ -std=c++11 -pthread -g -c Cache.cpp -o Cache.o

CacheItem: CacheItem.cpp
	g++ -std=c++11 -g -c CacheItem.cpp -o CacheItem.o

link: proxy Cache CacheItem
	g++ -std=c++11 -pthread -g proxy.o Cache.o CacheItem.o -o proxy

test: link
	./proxy 21000000

clean:
	rm -rf *.o proxy


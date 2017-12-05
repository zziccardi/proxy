
all: link

proxy: proxy.cpp
	g++ -std=c++11 -pthread -c proxy.cpp -o proxy.o

Cache: Cache.cpp
	g++ -std=c++11 -pthread -c Cache.cpp -o Cache.o

CacheItem: CacheItem.cpp
	g++ -std=c++11 -c CacheItem.cpp -o CacheItem.o

link: proxy Cache CacheItem
	g++ -std=c++11 -pthread proxy.o Cache.o CacheItem.o -o proxy

test: link
	./proxy 320000

clean:
	rm -rf *.o proxy


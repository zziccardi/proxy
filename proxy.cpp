
#include "proxy.hpp"

Cache cache;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <max-cache-size>" << endl;
        exit(EXIT_FAILURE);
    }

    cache.maxSize = stoi(argv[1]);
    
    // Create a TCP socket
    int mySocket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (mySocket == -1) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in myAddr;
    
    memset(&myAddr, 0, sizeof myAddr);
    
    myAddr.sin_family      = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Use this machine's IP address
    myAddr.sin_port        = htons(0);          // Use any free port
    
    // Bind the IP address and port to the socket
    if (bind(mySocket, (sockaddr *) &myAddr, sizeof myAddr) == -1) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }
    
    const int backlogSize = 50;
    
    // Listen for connection requests
    if (listen(mySocket, backlogSize) == -1) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }
    
    string hostName = getProxyHostName();
    
    cout << "Host name: " << hostName << endl;
    
    int port = getProxyPort(mySocket, myAddr);
    
    cout << "Port: " << port << endl;
    
    // Wait for connection requests from clients
    while (true) {
        struct sockaddr_in clientAddr;
        
        memset(&clientAddr, 0, sizeof clientAddr);
        
        socklen_t saLength = sizeof clientAddr;
        
        // Block until a connection request arrives.
        // Accept the connection request to the proxy's socket, create a new connected socket for
        // the client, and return a file descriptor referring to that socket.
        int clientSocket = accept(mySocket, (sockaddr *) &clientAddr, &saLength);
        
        if (clientSocket == -1) {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }
        
        pthread_t thread;
        
        int r = pthread_create(&thread, NULL, requestHandler, (void *) &clientSocket);
        
        if (r != 0) {
            errno = r;
            perror("pthread_create() failed");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Read the HTTP request from the client, parse the URL from it, and check if the requested object is in the cache. If so, just send the cached copy. If not, forward the request to the server, cache the response, parse it, and forward it to the client.
 * @param fd - the client's socket file descriptor
 */
void* requestHandler(void* fd) {
    int clientSocket = *((int *) fd);
    
    // The exact buffer size shouldn't matter since it's a stream socket
    // https://stackoverflow.com/questions/2862071/how-large-should-my-recv-buffer-be-when-calling-recv-in-the-socket-library
    const int requestBufferSize = 2048;
    
    char requestBuffer[requestBufferSize];
    
    int byteCount;
    
    // Read the request from the client
    if ((byteCount = recv(clientSocket, requestBuffer, requestBufferSize - 1, 0)) == -1) {
        perror("recv() failed");
        exit(EXIT_FAILURE);
    }
    
    requestBuffer[byteCount] = '\0';
    
    // The client closed the connection without sending any data
    if (byteCount == 0) {
        
        
        // TODO: Do I need to do anything here, or can I just kill the thread?
        
        
        pthread_exit(NULL);
    }
    
    cout << endl << requestBuffer;
    
    string requestString = requestBuffer;
    string url           = requestString.substr(0, requestString.find("\r"));
    
    url = url.substr(4);
    url = url.substr(0, url.find(" HTTP"));
    
    //cout << "URL: " << url << endl << endl;
    
    string fullResponse = "";
    
    // If the response is not already cached, forward the request to the server, cache the response,
    // and return it from talkToServer
    if ((fullResponse = cache.access(url)) == "") {
        cout << "Cache miss: " << url << endl;
        
        fullResponse = talkToServer(requestString, url);
    }
    else {
        cout << "Cache hit:  " << url << endl;
    }
    
    // Use the data method instead of the c_str method so it works with binary data, which may contain
    // the null terminator anywhere. Because it's not an ordinary null-terminated C string, it's
    // necessary to keep track of the length.
    const char* fullResponseBuffer = fullResponse.data();
    
    int fullLength = fullResponse.length();
    int bytesSent  = 0;
    int bytesLeft  = fullLength;
    int r          = -1;
    
    // While the response has not been fully sent (large files won't be sent all at once)
    // https://beej.us/guide/bgnet/output/html/multipage/advanced.html#sendall
    while (bytesSent < fullLength) {
        // Send the server's response to the client
        r = send(clientSocket, fullResponseBuffer + bytesSent, bytesLeft, 0);
        
        if (r == -1) {
            perror("send() failed");
            exit(EXIT_FAILURE);
        }
        
        bytesSent += r;
        bytesLeft -= r;
    }
    
    cout << "Full length: " << fullLength << endl;
    cout << "Bytes sent:  " << bytesSent  << endl;
    cout << "Bytes left:  " << bytesLeft  << endl;
    cout << "r:           " << r          << endl;
    
    // Close the client's socket file descriptor
    if (close(clientSocket) == -1) {
        perror("close() failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * Forward the client's request to the server, cache the server's response, and return it to this function's caller. (This function is only called if a response isn't already cached.)
 * @param request - the client's full request
 * @param url - the URL of the server as specified in the client's request
 * @return fullResponse - the response from the server
 */
string talkToServer(const string& request, const string& url) {
    // Get a substring of the request containing only the headers (everything after line 1)
    string requestHeaders = request.substr(request.find("\r\n") + 2);
    
    //cout << "Request headers:\n" << requestHeaders << endl;
    
    int hostIndex = requestHeaders.find("Host:") + 6;
    
    string hostName = "";
    
    // Extract the host name from the string containing all the headers
    for (int i = hostIndex; i < requestHeaders.size(); i++) {
        if (requestHeaders[i] == '\r') {
            break;
        }
        
        hostName += requestHeaders[i];
    }
    
    //cout << "Host name: " << hostName << endl;
    
    // Parse the path from the URL
    string path = url.substr(url.find(hostName) + hostName.size());
    
    //cout << "Path: " << path << endl;
    
    string newRequestLine = "GET " + path + " HTTP/1.1\r\n";
    string newRequest     = newRequestLine + requestHeaders;
    
    // Set up a connection between this proxy and the server
    int serverSocket = connectToServer(hostName);
    
    const char* newRequestBuffer = newRequest.c_str();
    
    // Send the new GET request to the server
    if (send(serverSocket, newRequestBuffer, strlen(newRequestBuffer) + 1, 0) == -1) {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }
    
    // 2 ^ 17
    const int responseBufferSize = 131072;
    
    string fullResponse;
    
    // Loop until all of the response data is received
    while (true) {
        char responseBuffer[responseBufferSize];
        
        // Block until a message (part or all of the response) arrives from the server, then
        // receive it
        int byteCount = recv(serverSocket, responseBuffer, responseBufferSize - 1, 0);
        
        if (byteCount == -1) {
            perror("recv() failed");
            exit(EXIT_FAILURE);
        }
        
        // The server closed the connection, so all data has been sent
        if (byteCount == 0) {
            break;
        }
        
        cout << endl << "***** NEW MESSAGE RECEIVED *****" << endl;
        cout << "Byte count: " << byteCount << endl << endl;
        
        responseBuffer[byteCount] = '\0';
        
        cout << endl << responseBuffer << endl << endl;
        
        // Append the part of the response that was just received to the string that will contain the
        // entire response once all parts are received
        fullResponse += responseBuffer;
    }
    
    // Close the server's socket file descriptor
    if (close(serverSocket) == -1) {
        perror("close() failed");
        exit(EXIT_FAILURE);
    }
    
    // Create a new CacheItem for the resource specified by the URL, containing the server's response
    CacheItem item(url, fullResponse);
    
    // Cache the response
    cache.insert(item);
    
    return fullResponse;
}

/**
 * Connect the proxy to the server specified by the client
 * https://beej.us/guide/bgnet/output/html/multipage/getaddrinfoman.html
 * @param hostName - the host name of the server
 * @return serverSocket - the socket file descriptor used for the connection to the server
 */
int connectToServer(const string& hostName) {
    struct addrinfo  hints;
    struct addrinfo* results = nullptr;
    struct addrinfo* current = nullptr;
    
    memset(&hints, 0, sizeof hints);
    
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int gaiResult;
    
    // Get one or more addrinfo structures (results) about the host specified by hostName, which
    // is the server the client wants to reach through the proxy
    if ((gaiResult = getaddrinfo(hostName.c_str(), "http", &hints, &results)) != 0) {
        cerr << gai_strerror(gaiResult) << endl;
        exit(EXIT_FAILURE);
    }
    
    int serverSocket;
    
    // Loop through the results returned and connect to the first we can
    for (current = results; current != nullptr; current = current->ai_next) {
        // Create a socket for this proxy (acting as a client) to connect to the server the client
        // wants to reach
        serverSocket = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        
        if (serverSocket == -1) {
            perror("socket() failed");
            continue;
        }
        
        // Connect the server socket to the address specified
        if (connect(serverSocket, current->ai_addr, current->ai_addrlen) == -1) {
            perror("connect() failed");
            close(serverSocket);
            continue;
        }
        
        // We successfully connected
        break;
    }
    
    // If we looped through all the results without being able to connect
    if (current == nullptr) {
        cerr << "Failed to connect" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Free the memory allocated for the results
    freeaddrinfo(results);
    
    return serverSocket;
}

/**
 * Get the host name the proxy is running on
 * @return hostName
 */
string getProxyHostName() {
    const int localHostNameSize = 1024;
    
    char localHostName[localHostNameSize];
    
    // Get the proxy's local host name
    if (gethostname(localHostName, localHostNameSize) == -1) {
        perror("gethostbyname() failed");
        exit(EXIT_FAILURE);
    }
    
    struct addrinfo  hints;
    struct addrinfo* results = nullptr;
    struct addrinfo* current = nullptr;
    
    memset(&hints, 0, sizeof hints);
    
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_CANONNAME;
    
    int gaiResult;
    
    // Get one or more addrinfo structures (results), which contain address information about the
    // host (this proxy) specified by localHostName
    if ((gaiResult = getaddrinfo(localHostName, "http", &hints, &results)) != 0) {
        cerr << gai_strerror(gaiResult) << endl;
        exit(EXIT_FAILURE);
    }
    
    string hostName;
    
    // Loop through the results returned
    for (current = results; current != nullptr; current = current->ai_next) {
        // Just get the first one
        hostName = current->ai_canonname;
        break;
    }
    
    // If we looped through all the results without getting a host name
    if (current == nullptr) {
        cerr << "Failed to get a host name" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Free the memory allocated for the results
    freeaddrinfo(results);
    
    return hostName;
}

/**
 * Get the port the proxy is running on
 * @param socket - the socket file descriptor
 * @param sa - the sockaddr structure to use
 * @return port
 */
int getProxyPort(const int& socket, const struct sockaddr_in& sa) {
    socklen_t saLength = sizeof sa;
    
    // Store the locally bound name of the socket in the sockaddr structure
    if (getsockname(socket, (sockaddr *) &sa, &saLength) == -1) {
        perror("getsockname() failed");
        exit(EXIT_FAILURE);
    }
    
    int port = ntohs(sa.sin_port);
    
    return port;
}


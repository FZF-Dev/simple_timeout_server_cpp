#ifndef SERVER
#define SERVER

#include <iostream>
#include <string>
#include <thread>
#include <map>
#include <ctype.h>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "ws2_32.lib")

int getTimeout(char* reqUrl) {
    int timeout = 3000;
    std::string extracted = "";
    bool reachedQueryString = false;
    
    try {
        for (int i = 0; i < strlen(reqUrl); i++) {
            if (i > 1999) break;

            char current = reqUrl[i];

            if (current == '?') {//find the beginning of the query string
                reachedQueryString = true;
            }
            else if (
                i >= 1 &&
                reachedQueryString &&
                (reqUrl[i - 1] == '?' ||
                reqUrl[i - 1] == '&') &&
                reqUrl[i] == 't' &&
                reqUrl[i + 1] == 'i' &&
                reqUrl[i + 2] == 'm' &&
                reqUrl[i + 3] == 'e' &&
                reqUrl[i + 4] == '='
                ) {//get the two chars next to the equal sign, but only if they are ints
                extracted += isdigit(reqUrl[i + 5]) ? reqUrl[i + 5] : (char)0;
                extracted += isdigit(reqUrl[i + 6]) ? reqUrl[i + 6] : (char)0;
                break;//break out of the loop, we have what we need
            }
        }
    }
    catch (...) {
        extracted = 3;
    }

    //parse the extracted value to int
    //If extracted is an empty string we either didn't find the value in the query string,
    //or it wasn't a number.In this case, default to 3 seconds(3000 ms);
    std::cout << extracted << std::endl;
    int extractedInt = std::stoi(extracted == "" ? "3" : extracted);
    if (!(extractedInt < 0) && !(extractedInt > 60)) {
        return extractedInt * 1000;
    }
    else return 3000;
}

void threadHandleResponse(int* nSocket) {
        std::string successResponse = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 0\r\n"
            "\r\n"
            "Success\r\n";

        std::string failureResponse = 
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 0\r\n"
            "\r\n"
            "Bad Request\r\n";

        int nLen = sizeof(struct sockaddr);
        int clientSocket = accept(*nSocket, NULL, &nLen);
        
        char buff[2000];
        int receivingErrorCode = recv(clientSocket, buff, 2000, 0);
        if (receivingErrorCode >= 0) {
            Sleep(getTimeout(buff));
            send(clientSocket, successResponse.c_str(), successResponse.length(), 0);
        }
        else {
            std::cout << "error receiving request url" << std::endl;
            send(clientSocket, successResponse.c_str(), successResponse.length(), 0);
        }

        closesocket(clientSocket);
        //std::thread currentRequestThread = std::thread();
}

class Server {
    fd_set fr, fw, fe;
    int nSocket;
    int PORT;
    int connectionQueueLength;
    bool run = true;

public:
    Server() {
        this->PORT = 9999;
        this->connectionQueueLength = SOMAXCONN;
        initServer();
    }

    Server(int PORT, int connectionQueueLength) {
        this->PORT = PORT;
        this->connectionQueueLength = connectionQueueLength;
        initServer();
    }

    ~Server() {
        WSACleanup();
    }

    void stop() {
        this->run = false;
        WSACleanup();
    }

    void start() {
        //Listen for request from client
        int errorCode = listen(nSocket, this->connectionQueueLength);
        if (errorCode < 0) {
            std::cout << "failed to start listening on local port..." << std::endl;
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        else {
            std::cout << "listening on port " << this->PORT << std::endl;
        }

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        while (this->run) {
            FD_ZERO(&this->fr);
            FD_ZERO(&this->fw);
            FD_ZERO(&this->fe);

            FD_SET(this->nSocket, &this->fr);
            FD_SET(this->nSocket, &this->fe);

            //cout << "before select call: " << fr.fd_count << endl;

            //Keep waiting for new requests and proceed with the requests
            int waiting = select(this->nSocket + 1, &this->fr, &this->fw, &this->fe, &tv);
            if (waiting > 0) {
                //When someone connects or communicates with a message
                //over a dedicated connection

                std::cout << "request received, processing..." << std::endl;
                //Process the request.
                processNewRequest();
            }
            else if (waiting == 0) {
                //No connection or any communication request made or you can say
                //that none of the socket descriptors are ready
                //cout << "nothing on port: " << PORT << endl;
            }
            else {
                //failed to get request
                std::cout << "failed to check socket bound to PORT: " << this->PORT << std::endl;
                WSACleanup();
            }
            Sleep(100);
        }
    }

private:
    bool initServer() {
        if (startWSA() && initSocket()) return true;
        return false;
    }

    bool startWSA() {
        WSADATA ws;
        int wsaStartCode = (int)WSAStartup(MAKEWORD(2, 2), &ws);
        if (wsaStartCode < 0) {
            std::cout << "WSA failed to initialize... error code: " << wsaStartCode << std::endl << "exiting..." << std::endl;
            return false;
        }
        else {
            return true;
        }
    }

    bool initSocket() {
        bool success = true;
        int status;
        struct sockaddr_in srv;

        //Initialize the socket
        this->nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (this->nSocket < 0) {
            std::cout << "failed to open socket" << std::endl << "Exiting..." << std::endl;
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        else {
            std::cout << "Socket opened successfully (" << this->nSocket << ")" << std::endl;
        }

        //Initialize the environment for sockaddr structure
        srv.sin_family = AF_INET;
        srv.sin_port = htons(PORT);
        srv.sin_addr.s_addr = INADDR_ANY;//could use: inet_addr("127.0.0.1");
        memset(srv.sin_zero, 0, 8);

        //setsockopt
        int nOptVal = 0;
        int nOptLen = sizeof(nOptVal);
        status = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOptLen, nOptLen);
        if (status != 0) {
            std::cout << "setsockopt failed..." << std::endl << "exiting..." << std::endl;
            WSACleanup();
            success = false;
        }

        //Bind the socket to a local port
        status = bind(nSocket, (sockaddr*)&srv, sizeof(sockaddr));
        if (status != 0) {
            std::cout << "failed to bind to port" << std::endl;
            WSACleanup();
            success = false;
        }

        return success;
    }

    void processNewRequest() {
        //new connection request
        if (FD_ISSET(this->nSocket, &this->fr)) {
            std::thread currentRequestThread (threadHandleResponse, &this->nSocket);
            currentRequestThread.detach();
        }
    }
};

#endif // !SERVER
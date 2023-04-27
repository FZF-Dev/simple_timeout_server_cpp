#ifndef SERVER
#define SERVER

#include <iostream>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "ws2_32.lib")

//using socklen_t = int;
//#define PORT 9909

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
            std::string response = "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 0\r\n"
                "\r\n"
                "successful connection\r\n";

            int nLen = sizeof(struct sockaddr);
            int clientSocket = accept(nSocket, NULL, &nLen);
            send(clientSocket, response.c_str(), response.length(), 0);
            Sleep(1);
            closesocket(clientSocket);
        }
    }

};

#endif // !SERVER
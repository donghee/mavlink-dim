#ifndef DIM_H
#define DIM_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>   // Standard input/output definitions
#include <pthread.h> // This uses POSIX Threads

#include <iostream>
#include <stdexcept>

#include <signal.h>
#include <string.h>
#include <errno.h>

#define SERVER_IP "192.168.88.26"

namespace dronemap
{
    class DimSocket
    {
    private:
        int _fd{};
        int server_fd{};
        struct sockaddr_in _addr{};
        struct sockaddr_in server_addr{};
        uint8_t _handshake_type;
        pthread_mutex_t  lock;

        enum SocketStatus {
             CONNECTED,
             DISCONNECTED
        };
        SocketStatus _connection_status{DISCONNECTED};


    public:
        explicit DimSocket(uint16_t port, bool bind = false) {
            // Start mutex
            int result = pthread_mutex_init(&lock, NULL);
            if ( result != 0 )
            {
                printf("\n mutex init failed\n");
                throw 1;
            }

            try {
                open(SERVER_IP, port, bind);
            } catch (...) {
                std::cout << " catch runtime error (...) " << std::endl;
                close();
            }
        };
        ~DimSocket() {
            pthread_mutex_destroy(&lock);
            close();
        };
        void open(const char *ip, unsigned long port, bool bind = false);
        int bind_();
        int listen();
        int connect();
        void disconnect();
        bool is_connected() const { return _connection_status == CONNECTED; };
        int handshake();
        int send(uint16_t size, uint8_t* data);
        int recv(int16_t* size, uint8_t* data);
        void close();
    };
}

#endif

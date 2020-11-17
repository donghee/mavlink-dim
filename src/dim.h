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

#include <poll.h>

//#define SERVER_IP "192.168.88.26"
#define SERVER_IP "10.243.61.157"

#define DIM_TIMEOUT 10

namespace dronemap
{
    class DimSocket
    {
    private:
        int _fd{};
        int _server_fd{};
        struct sockaddr_in _addr{};
        struct sockaddr_in _server_addr{};
        uint8_t _handshake_type;
        pthread_mutex_t  lock;
        pthread_cond_t send_cond;
        pthread_cond_t recv_cond;

        enum SocketStatus {
             CONNECTED,
             DISCONNECTED
        };

        SocketStatus _connection_status{DISCONNECTED};

        volatile bool on_read{false};
        volatile bool on_write{false};

        bool _bind = false;

        int _tls_read_error = false;
        int _tls_write_error = false;

        //poll
        struct pollfd fds[2];

      public:
        explicit DimSocket(uint16_t port, bool bind = false) {
            // init pthread mutex, cond
            if (pthread_mutex_init(&lock, NULL)) {
                printf("\n mutex init failed\n");
                throw 1;
            }

            if (pthread_cond_init(&send_cond, NULL)) {
                printf("\n send_cond init failed\n");
                throw 1;
            }

            if (pthread_cond_init(&recv_cond, NULL)) {
                printf("\n recv_cond init failed\n");
                throw 1;
            }

            _bind = bind;

            try {
                open(SERVER_IP, port, bind);
            } catch (...) {
                std::cout << " catch runtime error (dim open) " << std::endl;
                close();
                throw std::runtime_error(strerror(errno));
            }
        };

        ~DimSocket() {
            pthread_cond_destroy(&recv_cond);
            pthread_cond_destroy(&send_cond);
            pthread_mutex_destroy(&lock);
            close();
        };

        void open(const char *ip, unsigned long port, bool bind = false);
        int bind_();
        int listen();
        int accept();
        int connect();

        bool is_connected() const { return _connection_status == CONNECTED; };
        bool is_writing();
        bool is_reading();
        int handshake();
        int send(uint16_t size, uint8_t* data);
        int recv(int16_t* size, uint8_t* data);
        int tls_close_notify();
        int tls_close();
        int close();
        void power_off();

        void init_poll();

        auto descriptor()
        { return _fd; }

        auto poll_descriptor()
        { return fds; }


    };
}

#endif

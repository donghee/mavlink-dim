#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#include <iostream>

#include <cstdlib>
#include <stdio.h>   // Standard input/output definitions
#include <unistd.h>  // UNIX standard function definitions
#include <fcntl.h>   // File control definitions
#include <termios.h> // POSIX terminal control definitions
#include <pthread.h> // This uses POSIX Threads
#include <signal.h>

#include <stdint.h>

#include <mavlink.h>

namespace dronemap
{
class SerialPort
{
  private:
    int _fd{};
    const char* uart_name;
    int baudrate;
    bool is_open;

    pthread_mutex_t  lock;

  public:

    SerialPort(const char *uart_name_ , int baudrate_)
    {
        // Initialize attributes
        is_open = false;

        uart_name = (char*)"/dev/ttyUSB0";
        baudrate  = 57600;

        uart_name = uart_name_;
        baudrate  = baudrate_;

        // Start mutex
        int result = pthread_mutex_init(&lock, NULL);
        if ( result != 0 )
        {
            printf("\n mutex init failed\n");
            throw 1;
        }

        _fd = open(uart_name);

        // Check success
        if (_fd == -1)
        {
            printf("failure, could not open port.\n");
            throw EXIT_FAILURE;
        }
    };
    ~SerialPort()
    {
        pthread_mutex_destroy(&lock);
        close();
    };

    int open(const char* port);
    bool connect(int baud, int data_bits, int stop_bits, bool parity, bool hardware_control);
    int send(uint16_t size, uint8_t* data);
    int recv(int16_t* size, uint8_t* data);
    int read(uint8_t &cp);
    int read_message(mavlink_message_t &message);
    int write(char *buf, unsigned len);
    void close();
};
}

#endif // SERIAL_PORT_H_

#include <serial_port.h>

using namespace dronemap;

int SerialPort::open(const char* port)
{
    int fd = -1;
    // Open serial port
    // O_RDWR - Read and write
    // O_NOCTTY - Ignore special chars like CTRL-C
    fd = ::open(port, O_RDWR | O_NOCTTY | O_NDELAY);

    // Check for Errors
    if (fd == -1)
    {
        /* Could not open the port. */
        return(-1);
    }

    // Finalize
    else
    {
        fcntl(fd, F_SETFL, 0);
    }

    bool success = connect(baudrate, 8, 1, false, false);
    if (!success)
    {
        printf("failure, could not configure port.\n");
        throw EXIT_FAILURE;
    }

    printf("Connected to %s with %d baud, 8 data bits, no parity, 1 stop bit (8N1)\n", uart_name, baudrate);

    is_open = true;

    // Done!
    return fd;
}

bool SerialPort::connect(int baud, int data_bits, int stop_bits, bool parity, bool hardware_control)
{
    // Check file descriptor
    if(!isatty(_fd))
    {
        fprintf(stderr, "\nERROR: file descriptor %d is NOT a serial port\n", _fd);
        return false;
    }

    // Read file descritor configuration
    struct termios  config;
    if(tcgetattr(_fd, &config) < 0)
    {
        fprintf(stderr, "\nERROR: could not read configuration of fd %d\n", _fd);
        return false;
    }


    // Input flags - Turn off input processing
    // convert break to null byte, no CR to NL translation,
    // no NL to CR translation, don't mark parity errors or breaks
    // no input parity check, don't strip high bit off,
    // no XON/XOFF software flow control
    config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                        INLCR | PARMRK | INPCK | ISTRIP | IXON);

    // Output flags - Turn off output processing
    // no CR to NL translation, no NL to CR-NL translation,
    // no NL to CR translation, no column 0 CR suppression,
    // no Ctrl-D suppression, no fill characters, no case mapping,
    // no local output processing
    config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
                         ONOCR | OFILL | OPOST);

    #ifdef OLCUC
        config.c_oflag &= ~OLCUC;
    #endif

    #ifdef ONOEOT
        config.c_oflag &= ~ONOEOT;
    #endif

    // No line processing:
    // echo off, echo newline off, canonical mode off,
    // extended input processing off, signal chars off
    config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

    // Turn off character processing
    // clear current char size mask, no parity checking,
    // no output processing, force 8 bit input
    config.c_cflag &= ~(CSIZE | PARENB);
    config.c_cflag |= CS8;

    // One input byte is enough to return from read()
    // Inter-character timer off
    config.c_cc[VMIN]  = 1;
    config.c_cc[VTIME] = 10; // was 0

    // Get the current options for the port
    ////struct termios options;
    ////tcgetattr(_fd, &options);

    // Apply baudrate
    switch (baud)
    {
        case 1200:
            if (cfsetispeed(&config, B1200) < 0 || cfsetospeed(&config, B1200) < 0)
            {
                fprintf(stderr, "\nERROR: Could not set desired baud rate of %d Baud\n", baud);
                return false;
            }
            break;
        case 1800:
            cfsetispeed(&config, B1800);
            cfsetospeed(&config, B1800);
            break;
        case 9600:
            cfsetispeed(&config, B9600);
            cfsetospeed(&config, B9600);
            break;
        case 19200:
            cfsetispeed(&config, B19200);
            cfsetospeed(&config, B19200);
            break;
        case 38400:
            if (cfsetispeed(&config, B38400) < 0 || cfsetospeed(&config, B38400) < 0)
            {
                fprintf(stderr, "\nERROR: Could not set desired baud rate of %d Baud\n", baud);
                return false;
            }
            break;
        case 57600:
            if (cfsetispeed(&config, B57600) < 0 || cfsetospeed(&config, B57600) < 0)
            {
                fprintf(stderr, "\nERROR: Could not set desired baud rate of %d Baud\n", baud);
                return false;
            }
            break;
        case 115200:
            if (cfsetispeed(&config, B115200) < 0 || cfsetospeed(&config, B115200) < 0)
            {
                fprintf(stderr, "\nERROR: Could not set desired baud rate of %d Baud\n", baud);
                return false;
            }
            break;

        // These two non-standard (by the 70'ties ) rates are fully supported on
        // current Debian and Mac OS versions (tested since 2010).
        case 460800:
            if (cfsetispeed(&config, B460800) < 0 || cfsetospeed(&config, B460800) < 0)
            {
                fprintf(stderr, "\nERROR: Could not set desired baud rate of %d Baud\n", baud);
                return false;
            }
            break;
        case 921600:
            if (cfsetispeed(&config, B921600) < 0 || cfsetospeed(&config, B921600) < 0)
            {
                fprintf(stderr, "\nERROR: Could not set desired baud rate of %d Baud\n", baud);
                return false;
            }
            break;
        default:
            fprintf(stderr, "ERROR: Desired baud rate %d could not be set, aborting.\n", baud);
            return false;

            break;
    }

    // Finally, apply the configuration
    if(tcsetattr(_fd, TCSAFLUSH, &config) < 0)
    {
        fprintf(stderr, "\nERROR: could not set configuration of fd %d\n", _fd);
        return false;
    }

    // Done!
    return true;
}

void SerialPort::close()
{
    printf("CLOSE PORT\n");

    int result = ::close(_fd);

    if ( result )
    {
        fprintf(stderr,"WARNING: Error on port close (%i)\n", result );
    }

    is_open = false;

    printf("\n");
}

auto SerialPort::send(uint16_t size, uint8_t* data) -> int {
    if (is_open != true)
    {
        std::cout << "[SerialPort] send failed : not connected." << std::endl;
        return -1;
    }

    int result = write((char*)data, size);
    if (result < 0) {
        std::cout << "write() : Fail " << result << std::endl;
        close();
        throw std::runtime_error(strerror(errno));
    } else {
        // std::cout << "write() : Success " << result << std::endl;
    }
    return 0;
}

auto SerialPort::recv(int16_t* size, uint8_t* data) -> int {
    uint8_t          msgReceived = false;
    mavlink_status_t status;
    mavlink_message_t message;

   if (is_open != true)
   {
      std::cout << "[SerialPort] recv failed : not connected." << std::endl;
      return -1;
    }

   // int _size = ::read(_fd, data, sizeof(data));
   // *size = _size;
   // return _size;

   uint8_t cp;

   while(!msgReceived) {
       int result = read(cp);

       if (result < 0) {
           return -1;
       } else {
           // the parsing
           msgReceived = mavlink_parse_char(MAVLINK_COMM_0, cp, &message, &status);
       }
   }

   *size = mavlink_msg_to_send_buffer(data, &message);

   return *size;
}

int SerialPort::read(uint8_t &cp)
{
    pthread_mutex_lock(&lock);
    int result = ::read(_fd, &cp, 1);
    pthread_mutex_unlock(&lock);

    return result;
}

int SerialPort::write(char *buf, unsigned int len)
{
    pthread_mutex_lock(&lock);
    const int bytesWritten = ::write(_fd, buf, len);
    // Wait until all data has been written
    tcdrain(_fd);
    pthread_mutex_unlock(&lock);

    return bytesWritten;
}

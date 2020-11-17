#include "dim.h"

extern "C" {
    #include "kse_ubuntu.h"
}

using namespace dronemap;

struct kse_power_t {
    uint8_t abVer[3];
    uint8_t abSerialNumber[8];
    uint8_t bLifeCycle;
    uint8_t bPinType;
    uint8_t bMaxPinRetryCount;
    uint16_t usMaxKcmvpKeyCount;
    uint16_t usMaxCertKeyCount;
    uint16_t usMaxIoDataSize;
    uint16_t usInfoFileSize;
};

static void show_kse_power_info(kse_power_t kse_power) {
    printf("  * Version          : %X.%02X.%02X\r\n", kse_power.abVer[0], kse_power.abVer[1], kse_power.abVer[2]);
    printf("  * Serial Number    : %02X%02X%02X%02X%02X%02X%02X%02X\r\n",
           kse_power.abSerialNumber[0], kse_power.abSerialNumber[1], kse_power.abSerialNumber[2],
           kse_power.abSerialNumber[3], kse_power.abSerialNumber[4], kse_power.abSerialNumber[5],
           kse_power.abSerialNumber[6], kse_power.abSerialNumber[7]);
    printf("  * MaxKcmvpKeyCount : %d\r\n", kse_power.usMaxKcmvpKeyCount);
    printf("  * MaxCertKeyCount  : %d\r\n", kse_power.usMaxCertKeyCount);
    printf("  * MaxIoDataSize    : %d\r\n", kse_power.usMaxIoDataSize);
    printf("  * FileSize         : %d\r\n", kse_power.usInfoFileSize);
}


void DimSocket::open(const char *ip, unsigned long port, bool bind) {

    // KSE POWER ON
    int16_t result;
    struct kse_power_t kse_power;
    result = _ksePowerOn(kse_power.abVer, &kse_power.bLifeCycle, kse_power.abSerialNumber, &kse_power.bPinType,
                      &kse_power.bMaxPinRetryCount, &kse_power.usMaxKcmvpKeyCount,
                      &kse_power.usMaxCertKeyCount, &kse_power.usMaxIoDataSize, &kse_power.usInfoFileSize);

    if (result == KSE_SUCCESS) {
        std::cout << "_ksePowerOn() : Success " << result << std::endl;
        show_kse_power_info(kse_power);
    } else if (result == KSE_FAIL_ALREADY_POWERED_ON) {
        std::cout << "_ksePowerOn() : Already power on " << result << std::endl;
        result = _ksePowerOff();
        if (result < 0) {
            std::cout << "_ksePowerOff() : Fail " << result << std::endl;
        } else {
            std::cout << "_ksePowerOff() : Success " << result << std::endl;
        }
        throw std::runtime_error(strerror(errno));
    } else {
        std::cout << "_ksePowerOn() : Fail " << result << std::endl;
        _ksePowerOff();
        throw std::runtime_error(strerror(errno));
    }

    _handshake_type = KSETLS_FULL_HANDSHAKE;

    if (bind != true) {
        // Client
        if (connect() == -1) {
            close(); // TODO
            throw std::runtime_error(strerror(errno));
        }
    } else {
        // Server
        if (bind_() == -1) {
            close(); // TODO ?
            throw std::runtime_error(strerror(errno));
        }
        if (listen() == -1) {
            close(); // TODO ?
            throw std::runtime_error(strerror(errno));
        }
    }
    return;
}


int DimSocket::bind_() {
    int result;

    // SOCKET OPEN
    if ( _server_fd = ::socket(AF_INET, SOCK_STREAM, 0); _server_fd == -1) {
        throw std::runtime_error(strerror(errno));
    }

    _server_addr.sin_family = AF_INET;
    _server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    _server_addr.sin_port = htons(4433);

    // BIND
    result = ::bind(_server_fd,
                    reinterpret_cast<struct sockaddr *>(&_server_addr),
                    sizeof(_server_addr));

    if (result == 0) {
        std::cout << "::bind() : Success " << result << std::endl;
    } else {
        std::cout << "::bind() : Fail " << result << std::endl;
        close(); // TODO resolve Aborted (core dumped)
        throw std::runtime_error(strerror(errno));
    }
    return result;
}

int DimSocket::listen() {
    int result;

    result = ::listen(_server_fd, 10);
    if (result < 0) {
        std::cout << "::listen() : Fail " << result << std::endl;
    }

    return result;
}

int DimSocket::accept() {
    int result;
    char sIpAddress[40];
    unsigned int client_addr_len = sizeof(_addr);

    _connection_status = DISCONNECTED;

    std::cout << "Wait for client.\r\n"  << std::endl;
    _fd = ::accept(_server_fd,
                 reinterpret_cast<struct sockaddr *>(&_addr),
                 &client_addr_len);

    if (_fd < 0)
    {
        std::cout << "::accept() : Fail " << _fd << std::endl;
	return _fd;
    }

    inet_ntop(AF_INET, &_addr.sin_addr.s_addr, sIpAddress,
              sizeof(sIpAddress));
    printf("Client(%s) connected.\r\n", sIpAddress);

    // TLS OPEN
    result = _ksetlsOpen(_fd, KSETLS_MODE_TLS, KSETLS_SERVER, 1, 0, 0, SESSION);
    if (result == KSE_SUCCESS) {
        std::cout << "_kseTlsOpen() : Success " << result << std::endl;
    } else {
        std::cout << "_kseTlsOpen() : Fail " << result << std::endl;
        throw std::runtime_error(strerror(errno));
    }

    // HANDSHAKE
    result = _ksetlsTlsServerHandshake(_fd);
    if (result == KSE_SUCCESS) {
        std::cout << "_ksetlsTlsServerHandshake() : Success " << result << std::endl;
    } else {
        std::cout << "_ksetlsTlsServerHandshake() : Fail " << result << std::endl;
    }

    _connection_status = CONNECTED;
    _tls_read_error = 0; // NO TLS ERROR
    _tls_write_error = 0; // NO TLS ERROR
    return result;
}

int DimSocket::connect() {
    // if (_connection_status == CONNECTED) {
    //     close();
    //     std::cout << "new connect()" << std::endl;
    // }

    // SOCKET OPEN
    if ( _fd = ::socket(PF_INET, SOCK_STREAM, 0); _fd == -1) {
        throw std::runtime_error(strerror(errno));
    }

    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    _addr.sin_port = htons(4433);

    int result;
    // SOCKET CONNECT
    result = ::connect(_fd,
                       reinterpret_cast<struct sockaddr *>(&_addr),
                       sizeof(_addr));

    if (result == 0) {
        std::cout << "::connect() : Success " << result << std::endl;
    } else {
        std::cout << "::connect() : Fail " << result << std::endl;
        // close(); // TODO resolve Aborted (core dumped)
        return result;
        // throw std::runtime_error(strerror(errno));
    }
    // TLS OPEN
    result = _ksetlsOpen(_fd, KSETLS_MODE_TLS, KSETLS_CLIENT, 1, 0, 0, SESSION);
    if (result == KSE_SUCCESS) {
        std::cout << "_kseTlsOpen() : Success " << result << std::endl;
    } else {
        std::cout << "_kseTlsOpen() : Fail " << result << std::endl;
        // close(); // TODO resolve Aborted (core dumped)
        return result;
        // throw std::runtime_error(strerror(errno));
    }
    result = handshake();
    if ( result < 0 ) {
        // close(); // TODO resolve Aborted (core dumped)
        // throw std::runtime_error(strerror(errno));
        return result;
        // throw std::runtime_error(strerror(errno));
    }

    _connection_status = CONNECTED;
    _tls_read_error = 0; // NO TLS ERROR
    _tls_write_error = 0; // NO TLS ERROR

    return result;
}

int DimSocket::handshake() {
    int result = _ksetlsTlsClientHandshake(_fd, _handshake_type);
    if (result == KSE_SUCCESS) {
        _handshake_type = KSETLS_ABBR_HANDSHAKE; // TOOD
        // _handshake_type = KSETLS_FULL_HANDSHAKE;
        std::cout << "_ksetlsTlsClientHandshake() : Success " << result << std::endl;
    } else {
        std::cout << "_ksetlsTlsClientHandshake() : Fail " << result << std::endl;
        return result;
    }

    return 0;
}

void DimSocket::init_poll() {
    memset(fds, 0, sizeof(fds));

    fds[0].events = POLLIN;
    fds[0].fd = _fd;
    fds[1].events = POLLIN;
    fds[1].fd = _server_fd;
}

bool DimSocket::is_writing() {
    return on_write;
}

bool DimSocket::is_reading() {
    return on_read;
}

auto DimSocket::send(uint16_t size, uint8_t* data) -> int {
    int result = 0;

    if (_connection_status != CONNECTED)
    {
        std::cout << "[DimSocket] send failed : not connected to a server." << std::endl;
        return -1;
    }

    if (_tls_read_error == -1) // HAS TLS ERROR?
    {
        // printf("\nDimSocket::send CHECK TLS ERROR\n");
        return 0;
    }

    while (on_read){};

    if (_tls_read_error == -1) // HAS TLS ERROR?
    {
        // printf("\nDimSocket::send CHECK TLS ERROR\n");
        return 0;
    }

    // printf("\nDimSocket::send before mutex lock\n");
    // pthread_mutex_lock(&lock);
    on_write = true;

    // if(size > 2048) {
    //     printf("\nDimSocket::send0\n");
    //     result = _ksetlsTlsWrite(_fd, data, 2048);
    //     if (result < 0) {
    //         std::cout << "_ksetlsTlsWrite() : Fail " << result <<  " " << size << std::endl;
    //         return -1;
    //     }
    //     result = _ksetlsTlsWrite(_fd, data+2048, size-2048);
    //     on_write = false;
    //     if (result < 0) {
    //         std::cout << "_ksetlsTlsWrite() : Fail " << result <<  " " << size << std::endl;
    //         return -1;
    //     }
    // } else
    {
        // printf("\nDimSocket::send0\n");
        result = _ksetlsTlsWrite(_fd, data, size);
        on_write = false;
        if (result < 0) {
            std::cout << "_ksetlsTlsWrite() : Fail " << result <<  " " << size << std::endl;
            _tls_write_error = -1;
        } else {
            _tls_write_error = 0;
        }
    }
    // printf("\nDimSocket::send1 %d\n", result);
    on_write = false;
    // pthread_mutex_unlock(&lock);
    // printf("\nDimSocket::send after mutex lock\n");

    return result;
}

auto DimSocket::recv(int16_t* size, uint8_t* data) -> int {
   int result = 0;

   if (_connection_status != CONNECTED)
   {
       std::cout << "[DimSocket] recv failed : not connected to a server." << std::endl;
       return -1;
   }

    // if (_tls_write_error == -1) // HAS TLS ERROR?
    // {
    //     printf("\nDimSocket::send CHECK TLS ERROR\n");
    //     return 0;
    // }

   // printf("\nDimSocket::recv before mutex lock\n");
   // pthread_mutex_lock(&lock);
   on_read = true;
   // if (poll(fds, 1, 30) > 0 && !on_write) {
   if (poll(fds, 1, DIM_TIMEOUT) > 0 && !on_write) {
       if (fds[0].revents & POLLIN) {
           // result = _ksetlsTlsRead(data, size, _fd);
           // printf("\nDimSocket::recv0\n");
           result = _ksetlsTlsRead(data, size, fds[0].fd);
           // printf("\nDimSocket::recv1 %d, %d\n", result, *size);
           if (result < 0) {
               std::cout << "_ksetlsTlsRead() : Fail " << result <<  " " << *size << std::endl;
               _tls_read_error = -1;
           } else {
               _tls_read_error = 0;
           }
       }
   }
   on_read = false;
   // pthread_mutex_unlock(&lock);
   // printf("\nDimSocket::recv after mutex lock\n");
   return result;
}

int DimSocket::tls_close_notify() {
    int result = 0;

    result = _ksetlsTlsCloseNotify(_fd);
    if (result < 0) {
        std::cout << "_ksetlsTlsCloseNotify() : Fail " << result << std::endl;
    } else {
        std::cout << "_ksetlsTlsCloseNotify() : Success " << result << std::endl;
    }
    return result;
}

int DimSocket::tls_close() {

    int result = 0;
    result = _ksetlsClose(_fd);
    if (result < 0) {
        std::cout << "_ksetlsClose() : Fail " << result << std::endl;
    } else {
        std::cout << "_ksetlsClose() : Success " << result << std::endl;
    }
    return result;
}

int DimSocket::close() {
    int result = 0;
    _connection_status = DISCONNECTED;

    // result = tls_close_notify(); //TODO

    ::close(_fd);
    printf("Client disconnected.\r\n");

    result = tls_close();
    return result;
}

void DimSocket::power_off() {
    int result = 0;

    result = _ksePowerOff();
    if (result < 0) {
        std::cout << "_ksePowerOff() : Fail " << result << std::endl;
    } else {
        std::cout << "_ksePowerOff() : Success " << result << std::endl;
    }
}

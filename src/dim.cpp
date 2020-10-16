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
    } else {
        std::cout << "_ksePowerOn() : Fail " << result << std::endl;
        close(); // TODO
        throw std::runtime_error(strerror(errno));
    }

    _handshake_type = KSETLS_FULL_HANDSHAKE;

    if (bind != true) {
        // Client CONNECT
        if (connect() == -1) {
            close(); // TODO
            throw std::runtime_error(strerror(errno));
        }
    } else {
        if (bind_() == -1) {
            close(); // TODO
            throw std::runtime_error(strerror(errno));
      }
    }
    return;
}

void DimSocket::disconnect() {
    ::close(_fd);
    _ksetlsClose(_fd);
    _connection_status = DISCONNECTED;
}

int DimSocket::bind_() {
    int result;

    // SOCKET OPEN
    if ( server_fd = ::socket(AF_INET, SOCK_STREAM, 0); server_fd == -1) {
    //if ( server_fd = ::socket(AF_INET, SOCK_DGRAM, 0); server_fd == -1) { //udp
        throw std::runtime_error(strerror(errno));
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(4433);

    // BIND
    result = ::bind(server_fd,
                    reinterpret_cast<struct sockaddr *>(&server_addr),
                    sizeof(server_addr));

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
    // LISTEN AND ACCEPT
    result = ::listen(server_fd, 10);
    if (result < 0) {
        std::cout << "::listen() : Fail " << result << std::endl;
        // close(); // TODO resolve Aborted (core dumped)
        throw std::runtime_error(strerror(errno));
    }

    char sIpAddress[40];
    unsigned int client_addr_len = sizeof(_addr);

    std::cout << "Wait for client.\r\n"  << std::endl;
    _fd = accept(server_fd,
                 reinterpret_cast<struct sockaddr *>(&_addr),
                 &client_addr_len);

    if (_fd < 0)
    {
        std::cout << "::accept() : Fail " << result << std::endl;
        // close(); // TODO resolve Aborted (core dumped)
        throw std::runtime_error(strerror(errno));
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

    // if (handshake() == -1 ) {
    //     close(); // TODO resolve Aborted (core dumped)
    //     throw std::runtime_error(strerror(errno));
    // }

    _connection_status = CONNECTED;

    return result;
}

int DimSocket::connect() {
    if (_connection_status == CONNECTED) {
        disconnect();
        std::cout << "new connect()" << std::endl;
    }

    // SOCKET OPEN
    if ( _fd = ::socket(PF_INET, SOCK_STREAM, 0); _fd == -1) {
      //if ( _fd = ::socket(PF_INET, SOCK_DGRAM, 0); _fd == -1) { // udp
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
        // std::cout << "::connect() : Success " << result << std::endl;
    } else {
        std::cout << "::connect() : Fail " << result << std::endl;
        close(); // TODO resolve Aborted (core dumped)
        throw std::runtime_error(strerror(errno));
    }
    // TLS OPEN
    result = _ksetlsOpen(_fd, KSETLS_MODE_TLS, KSETLS_CLIENT, 1, 0, 0, SESSION);
    if (result == KSE_SUCCESS) {
        std::cout << "_kseTlsOpen() : Success " << result << std::endl;
    } else {
        std::cout << "_kseTlsOpen() : Fail " << result << std::endl;
        // close(); // TODO resolve Aborted (core dumped)
        throw std::runtime_error(strerror(errno));
    }

    if (handshake() == -1 ) {
        // close(); // TODO resolve Aborted (core dumped)
        throw std::runtime_error(strerror(errno));
    }

    _connection_status = CONNECTED;

    return result;
}

int DimSocket::handshake() {
    int result = _ksetlsTlsClientHandshake(_fd, _handshake_type);
    if (result == KSE_SUCCESS) {
        _handshake_type = KSETLS_ABBR_HANDSHAKE;
        std::cout << "_ksetlsTlsClientHandshake() : Success " << result << std::endl;
    } else {
        std::cout << "_ksetlsTlsClientHandshake() : Fail " << result << std::endl;
        return -1;
    }
    return 0;
}

auto DimSocket::send(uint16_t size, uint8_t* data) -> int {
    if (_connection_status != CONNECTED)
    {
        std::cout << "[DimSocket] send failed : not connected to a server." << std::endl;
        return -1;
    }
    int result = -1;

    pthread_mutex_lock(&lock);
    if(size > 2048) {
        result = _ksetlsTlsWrite(_fd, data, 2048);
        if (result < 0) {
            std::cout << "_ksetlsTlsWrite() : Fail " << result <<  " " << size << std::endl;
            return -1;
        }
        result = _ksetlsTlsWrite(_fd, data+2048, size-2048);
        if (result < 0) {
            std::cout << "_ksetlsTlsWrite() : Fail " << result <<  " " << size << std::endl;
            return -1;
        }
        return 0;
    } else {
        result = _ksetlsTlsWrite(_fd, data, size);
        if (result < 0) {
            std::cout << "_ksetlsTlsWrite() : Fail " << result <<  " " << size << std::endl;
            return -1;
        }
    }
    pthread_mutex_unlock(&lock);

    return 0;
}

auto DimSocket::recv(int16_t* size, uint8_t* data) -> int {
   if (_connection_status != CONNECTED)
   {
      std::cout << "[DimSocket] recv failed : not connected to a server." << std::endl;
      return -1;
   }
   int result = -1;

   pthread_mutex_lock(&lock);
   result = _ksetlsTlsRead(data, size, _fd);
   pthread_mutex_unlock(&lock);

    if (result < 0) {
        std::cout << "_ksetlsTlsRead() : Fail " << result << " " << (*size) << std::endl;
        return -1;
        //close();
        //        throw std::runtime_error(strerror(errno));
    }

    return result;
}

void DimSocket::close() {
    int result = -1;

    result = _ksetlsTlsCloseNotify(_fd);
    if (result < 0) {
        std::cout << "_ksetlsTlsCloseNotify() : Fail " << result << std::endl;
    } else {
        std::cout << "_ksetlsTlsCloseNotify() : Success " << result << std::endl;
    }

    result = _ksePowerOff();
    if (result < 0) {
        std::cout << "_ksePowerOff() : Fail " << result << std::endl;
    } else {
        std::cout << "_ksePowerOff() : Success " << result << std::endl;
    }

    disconnect();
}

#include "dim_server.h"

extern "C" {
#include "kse_ubuntu.h"
}

/**
 * open socket and connect server or listen client
 *
 * @param ip IP adress to open
 * @param port port number to open
 * @return
 */
void DimServer::open(const char *ip, uint16_t port)
{

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

  // Server
  if (bind_() == -1) {
    close(); // TODO ?
    throw std::runtime_error(strerror(errno));
  }

  if (listen() == -1) {
    close(); // TODO ?
    throw std::runtime_error(strerror(errno));
  }

  return;
}

/**
 * open socket and bind ip on server
 *
 * @return
 */
int DimServer::bind_()
{
  int result;

  // SOCKET OPEN
  if (_server_fd = ::socket(AF_INET, SOCK_STREAM, 0); _server_fd == -1) {
    throw std::runtime_error(strerror(errno));
  }

  _server_addr.sin_family = AF_INET;
  _server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  _server_addr.sin_port = htons(_server_port);

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

/**
 * listen client on server
 *
 * @return
 */
int DimServer::listen()
{
  int result;

  result = ::listen(_server_fd, 10);

  if (result < 0) {
    std::cout << "::listen() : Fail " << result << std::endl;
  }

  return result;
}

/**
 * wait client and accept connect client on server
 *
 * @return
 */
int DimServer::accept()
{
  int result;
  char sIpAddress[40];
  unsigned int client_addr_len = sizeof(_addr);

  _connection_status = DISCONNECTED;

  std::cout << "Wait for client.\r\n"  << std::endl;
  _fd = ::accept(_server_fd,
                 reinterpret_cast<struct sockaddr *>(&_addr),
                 &client_addr_len);

  if (_fd < 0) {
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
    return -1;
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

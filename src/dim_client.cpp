#include "dim_client.h"

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
void DimClient::open(const char *ip, uint16_t port)
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

    // Client
  if (connect() == -1) {
    close(); // TODO
  }

  return;
}

/**
 * connect server in client
 *
 * @return
 */
int DimClient::connect()
{
  // SOCKET OPEN
  if (_fd = ::socket(PF_INET, SOCK_STREAM, 0); _fd == -1) {
    throw std::runtime_error(strerror(errno));
  }

  _addr.sin_family = AF_INET;
  _addr.sin_addr.s_addr = inet_addr(_server_ip);
  _addr.sin_port = htons(_server_port);

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

  if (result < 0) {
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

/**
 * TLS handshake with server (client side)
 *
 * @return 0 TLS handshake is successful, otherwise -1
 */
int DimClient::handshake()
{
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

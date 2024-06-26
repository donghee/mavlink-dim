#include "dim.h"

extern "C" {
#include "kse_ubuntu.h"
}

void DimSocket::show_kse_power_info(kse_power_t kse_power)
{
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

/**
 * initializing file descript to use poll()
 *
 * @return
 */
void DimSocket::init_poll()
{
  memset(fds, 0, sizeof(fds));

  fds[0].events = POLLIN;
  fds[0].fd = _fd;
  fds[1].events = POLLIN;
  fds[1].fd = _server_fd;

  //fcntl(_fd, F_SETFL, O_NONBLOCK);
}

/**
 * check on writing in DIM hardware
 *
 * @return true if on writing, otherwise false
 */
bool DimSocket::is_writing()
{
  return on_write;
}

/**
 * check on reading in DIM hardware
 *
 * @return true if on reading, otherwise false
 */
bool DimSocket::is_reading()
{
  return on_read;
}

/**
 * send data using TLS writing of DIM hardware
 *
 * @param size size of data to send
 * @param data to sent
 * @return 0 if send data is successful, otherwise -1
 */
auto DimSocket::send(uint16_t size, uint8_t *data) -> int
{
  int result = 0;

  if (_connection_status != CONNECTED) {
    std::cout << "[DimSocket] send failed : not connected to a server." << std::endl;
    return -1;
  }

  if (_tls_read_error == -1) { // HAS TLS ERROR?
    // printf("\nDimSocket::send CHECK TLS ERROR\n");
    return 0;
  }

  while (on_read) {};

  if (_tls_read_error == -1) { // HAS TLS ERROR?
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


/**
 * receive data using TLS reading of DIM hardware
 *
 * @param size size of received data
 * @param data received data
 * @return 0 if receive data is successful, otherwise -1
 */
auto DimSocket::recv(int16_t *size, uint8_t *data) -> int
{
  int result = 0;

  if (_connection_status != CONNECTED) {
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

/**
 * TLS write close notify
 *
 * @return 0 if close notify is successful, otherwise -1
 */
int DimSocket::tls_close_notify()
{
  int result = 0;

  result = _ksetlsTlsCloseNotify(_fd);

  if (result < 0) {
    std::cout << "_ksetlsTlsCloseNotify() : Fail " << result << std::endl;

  } else {
    std::cout << "_ksetlsTlsCloseNotify() : Success " << result << std::endl;
  }

  return result;
}

/**
 * TLS close
 *
 * @return 0 if tls close is successful, otherwise -1
 */
int DimSocket::tls_close()
{

  int result = 0;
  result = _ksetlsClose(_fd);

  if (result < 0) {
    std::cout << "_ksetlsClose() : Fail " << result << std::endl;

  } else {
    std::cout << "_ksetlsClose() : Success " << result << std::endl;
  }

  return result;
}

/**
 * close socket and disconnect client or server
 *
 * @return 0 if close socket is successful, otherwise -1
 */
int DimSocket::close()
{
  int result = 0;
  _connection_status = DISCONNECTED;

  // result = tls_close_notify();

  result = ::close(_fd);
  printf("Client disconnected.\r\n");

  result = tls_close();
  return result;
}

/**
 * power off DIM hardware
 *
 * @return 0 if DIM hardware is successful power off, otherwise -1
 */
int DimSocket::power_off()
{
  int result = 0;

  result = _ksePowerOff();

  if (result < 0) {
    std::cout << "_ksePowerOff() : Fail " << result << std::endl;

  } else {
    std::cout << "_ksePowerOff() : Success " << result << std::endl;
  }

  return result;
}

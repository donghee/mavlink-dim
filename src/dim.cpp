#include "dim.h"

extern "C" {
  #include "kse_ubuntu.h"
}

int DimSocket::power_on()
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

  return result;
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

int DimSocket::encrypt(uint8_t* abData, int abData_len, uint8_t* abIv, int abIv_len, uint8_t* abAuth, int abAuth_len, uint8_t* abData1, uint8_t* abTag, int abTag_len) {
    int16_t sRv;
    int i;
    uint16_t usSize0, usSize1;

   // Encrypt abData to abData1
    sRv = _kcmvpAriaGcm(abData1, abData, 256, KCMVP_ARIA128_KEY, 0, abIv, 16,
                        abAuth, 128, abTag, 16, ENCRYPT);
    return sRv;
}

int DimSocket::decrypt(int name, int key, const char* buffer, int buffer_len, uint8_t* plain_text) {
  uint8_t abIv[16], abAuth[128], abTag[16], encrypted_text[256];

  for(int i = 0 ; i < 16; i++) {
    abIv[i] = buffer[i];
  }

  for(int i = 0 ; i < 128; i++) {
    abAuth[i] = buffer[16+i];
  }

  for(int i = 0 ; i < 16; i++) {
    abTag[i] = buffer[16+128+i];
  }

  for(int i = 0 ; i < 256; i++) {
    encrypted_text[i] = buffer[16+128+16+i];
  }

  printf("\r\n");
  printf("dim_decrypt()\r\n");
  printf("  * Decrypted Plaintext \r\n    ");

  int ret;

  ret = _kcmvpAriaGcm(plain_text, encrypted_text, 256, KCMVP_ARIA128_KEY, 0, abIv, 16, abAuth, 128, abTag, 16, DECRYPT);
  for (int i = 0; i < 256; i++) {
    printf("%02X", plain_text[i]);

    if ((i < 255) && ((i + 1) % 32 == 0)) {
      printf("\r\n    ");
    }
  }
  printf("\n");
}

int DimSocket::get_key(uint8_t *abPubKey0, int abPubKey0_len) {
    int ret;

    // get key 0
    //uint8_t abPubKey0[64];
    uint16_t usSize0 = 0;
    //memset(&abPubKey0, 0, sizeof(abPubKey0));
    memset(&abPubKey0, 0, abPubKey0_len);
    ret = _kcmvpGetKey(abPubKey0, &usSize0, KCMVP_ARIA128_KEY, 0);
    if (ret == KSE_SUCCESS)
      printf("Success kcmvpGetKey: %d\n", ret);
    else
      printf("Error kcmvpGetKey: %d\n", ret);

    printf("kcmvpGetKey Size: %d\n", usSize0);
    printf("kcmvpGetKey: ");
    for (int i = 0; i < usSize0; i++) {
      printf("%02X", abPubKey0[i]);
    }

    printf("\r\n");
    return 0;
}

int DimSocket::set_key(const uint8_t *abPubKey0, int abPubKey0_len) {
    int ret;

    // set key 0
    ret = _kcmvpEraseKey(KCMVP_ARIA128_KEY, 0);
    if (ret == KSE_SUCCESS)
      printf("Success kcmvpEraseKey: %d\n", ret);
    else
      printf("Error kcmvpEraseKey: %d\n", ret);

    ret = _kcmvpPutKey(KCMVP_ARIA128_KEY, 0, (uint8_t *) abPubKey0, 16);
    if (ret == KSE_SUCCESS)
      printf("Success kcmvpPutKey: %d\n", ret);
    else
      printf("Error kcmvpPutKey: %d\n", ret);

    return 0;
}

int DimSocket::generate_key() {
    int16_t ret;

    ret = _kcmvpEraseKey(KCMVP_ARIA128_KEY, 0);
    if (ret == KSE_SUCCESS)
      printf("Success kcmvpEraseKey: %d\n", ret);
    else
      printf("Error kcmvpEraseKey: %d\n", ret);

    ret = _kcmvpGenerateKey(KCMVP_ARIA128_KEY, 0, NOT_USED);
    if (ret == KSE_SUCCESS )
      printf("Success kcmvpGenerateKey: %d\n", ret);
    else 
      printf("Error kcmvpGenerateKey: %d\n", ret);

    return ret;
}

int DimSocket::generate_random(uint8_t* abData, int abData_len) {
    int i;
    int16_t sRv;
    //uint8_t abData[256];

    sRv = _kcmvpDrbg(abData, 256);
    if (sRv == KSE_SUCCESS)
        printf("Success kcmvpDrbg: %d\n", sRv);
    else
    {
        printf("Error kcmvpDrbg: Fail(-0x%04X)\r\n", -sRv);
        return -1;
    }
    printf("  * Random Number :\r\n    ");
    for (i = 0; i < abData_len; i++)
    {
        printf("%02X", abData[i]);
        if ((i < 255) && ((i + 1) % 32 == 0))
            printf("\r\n    ");
    }
    printf("\r\n");

    return sRv;
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

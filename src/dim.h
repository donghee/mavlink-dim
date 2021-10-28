#ifndef DIM_H
#define DIM_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <pthread.h>

#include <iostream>
#include <stdexcept>

#include <signal.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>

#include <poll.h>

#define DIM_TIMEOUT 10

/**
 * \class DimSocket
 * \brief Provies DIM socket interface for MAVLink TLS Handshake and encryption and decryption using DIM
 */
class DimSocket
{
 protected:
  /** data struct for DIM Hardware status. */
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

  int _fd{};   /** client socket file descriptor */
  int _server_fd{};   /** server socket file descriptor */
  struct sockaddr_in _addr; /** client IP adress */
  struct sockaddr_in _server_addr; /** server IP adress */
  uint8_t _handshake_type; /** is full handshake or abbreviation handshake */

  pthread_mutex_t  lock;   /** mutex lock */

  enum SocketStatus {
    CONNECTED,
    DISCONNECTED
  };

  SocketStatus _connection_status{DISCONNECTED};

  volatile bool on_read{false};
  volatile bool on_write{false};

  const char* _server_ip;
  uint16_t _server_port;
  bool _bind;

  int _tls_read_error = false;
  int _tls_write_error = false;

  //poll
  struct pollfd fds[2];

  /** print KSE DIM hardware state */
  void show_kse_power_info(kse_power_t kse_power);

 public:

  /**
   * DimSocket Class constructor.
   * initilize mutex lock and open socket
   *
   * @return
   */
  DimSocket(const char* server_ip, uint16_t port, bool bind = false) {
    // init pthread mutex, cond
    if (pthread_mutex_init(&lock, NULL)) {
      printf("\n mutex init failed\n");
      throw 1;
    }

    _server_ip = server_ip;
    _server_port = port;
    _bind = bind;

    try {
      open(_server_ip, port, bind);
    } catch (...) {
      std::cout << " catch runtime error (dim open) " << std::endl;
      close();
//      throw std::runtime_error(strerror(errno));
    }
  };

  /**
   * DimSocket Class deconstructor.
   * destory mutex lock and socket.
   *
   * @return
   */
  ~DimSocket() {
    pthread_mutex_destroy(&lock);
    close();
  };
/*
  void open(const char *ip, uint16_t port, bool bind = false);
  */

  /**
   * check client is connected
   *
   * @return 0 if send client is connected, otherwise -1
   */
  bool is_connected() const { return _connection_status == CONNECTED; };
  bool is_writing();
  bool is_reading();

/*
  int handshake();
  */
  int send(uint16_t size, uint8_t* data);
  int recv(int16_t* size, uint8_t* data);
  int tls_close_notify();
  int tls_close();
  int close();

  int power_on();
  int power_off();

  void init_poll();

  /**
   * return client socket file descriptor
   *
   * @return client socket file descriptor
   */
  auto descriptor()
  { return _fd; }


  /**
   * return client and server file descriptor from init_poll() function
   *
   * @return array of file descriptors
   */
  auto poll_descriptor()
  { return fds; }


};

#endif

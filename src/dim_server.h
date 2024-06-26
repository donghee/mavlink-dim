#ifndef DIM_SERVER_H
#define DIM_SERVER_H

#include "dim.h"

/**
 * \class DimServer
 * \brief Provies DIM server interface for MAVLink TLS Handshake and encryption and decryption using DIM
 */
class DimServer : public DimSocket
{
 private:
 public:

  DimServer(const char* server_ip, uint16_t port)
	  : DimSocket {server_ip, port, true }
  {
    try {
      open(server_ip, port);
    } catch (...) {
      std::cout << " catch runtime error (dim open) " << std::endl;
      close();
//      throw std::runtime_error(strerror(errno));
    }
  }

  void open(const char *ip, uint16_t port);
  int bind_();
  int listen();
  int accept();
  //int connect();
};

#endif

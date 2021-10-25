#ifndef DIM_CLIENT_H
#define DIM_CLIENT_H

#include "dim.h"

/**
 * \class DimClient
 * \brief Provies DIM socket interface for MAVLink TLS Handshake and encryption and decryption using DIM
 */
class DimClient : public DimSocket
{
 public:

  DimClient(const char* server_ip, uint16_t port)
	  : DimSocket {server_ip, port, false }
  {
    try {
      open(server_ip, port);
    } catch (...) {
      std::cout << " catch runtime error (dim open) " << std::endl;
      close();
    }
  }

  void open(const char *ip, uint16_t port);
  int handshake();
  int connect();
};

#endif

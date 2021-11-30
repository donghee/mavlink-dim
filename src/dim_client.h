#ifndef DIM_CLIENT_H
#define DIM_CLIENT_H

#include "dim.h"

/**
 * \class DimClient
 * \brief DIM client interface for MAVLink TLS Handshake with DIM Server and encryption and decryption using DIM
 */
class DimClient : public DimSocket
{
 public:

  DimClient(): DimSocket{}
  {
    if (power_on() == -1) {
      std::cout << "power_on() error" << std::endl;
      return;
    }

    /*
    try {
      open(server_ip, port);
    } catch (...) {
      std::cout << " catch runtime error (dim open) " << std::endl;
      close();
    }
    */
  }

  void open(const char *ip, uint16_t port);
  int handshake();
  int connect();
};

#endif

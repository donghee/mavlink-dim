/**


 * @file dim_server.h
 
*
 * @brief DIM TLS Server Interface
 *
 * DIM TLS Server Interface for MAVLink TLS Handshake and encryption and decryption using DIM
 *
 * @author Donghee Park,   <dongheepark@gmail.com>
 *
 */


#ifndef DIM_SERVER_H
#define DIM_SERVER_H

#include "dim.h"

/**
 * \class DimServer
 * \brief DIM server interface for MAVLink TLS Handshake and encryption and decryption using DIM
 */
class DimServer : public DimSocket
{
 private:
 public:

  DimServer(const char* server_ip, uint16_t port)
      : DimSocket {}
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

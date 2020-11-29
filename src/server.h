/**
 * @file server.h
 *
 * @brief DIM TLS server
 *
 * TCP TLS server code for communication MAVLink endpoints in MC
 *
 * @author Donghee Park,   <dongheepark@gmail.com>
 *
 */

#include <iostream>
#include <thread>

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <dim.h>
#include <signal.h>
#include <serial_port.h>


#define BUFFER_LENGTH 8192

#include "readerwriterqueue.h"

void exit_app(int signum);

/**
 * \class MAVLinkTlsServer
 * \brief MAVLink TLS Server
 */
class MAVLinkTlsServer
{

 public:
  explicit MAVLinkTlsServer(Serial_Port * _port, DimSocket * _dim) {
      port = _port;
      dim = _dim;

      run = 1;
  }

  ~MAVLinkTlsServer() {
  }

  void debug_mavlink_msg_buffer(uint8_t *buffer, int buffer_size);

  // autopilot -> gcs
  void autopilot_read_message();

  int gcs_write_message();

  // gcs -> autopilot
  int gcs_read_message();

  void autopilot_write_message();

  void *start_autopilot_read_thread(void *args);

  void *start_gcs_write_thread(void *args);

  void *start_gcs_read_thread(void *args);

  void *start_autopilot_write_thread(void *args);

  int start_threads();

  void exit(int signum);

  sig_atomic_t run;

 private:
  moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_gcs;
  moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_autopilot;

  Serial_Port *port;
  DimSocket *dim;

  volatile int autopilot_writing_status = 0;
  volatile int dim_writing_status = 0;
};

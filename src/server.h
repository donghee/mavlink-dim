
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
 * \brief MAVLink TLS Server for Autopilot Mission Computer.  It connect with Autopilot and GCS using DIM TLS Socket.
 */
class MAVLinkTlsServer
{

 public:
   /**
   * MAVLinkTlsServer Class constructor.
   *
   * @return
   */
  explicit MAVLinkTlsServer(Serial_Port * _port, DimSocket * _dim) {
      port = _port;
      dim = _dim;

      run = 1;
  }

  /**
   * MAVLinkTlsServer Class deconstructor.
   *
   * @return
   */
  ~MAVLinkTlsServer() {
  }

  /**
   * print mavlink bytes for debugging
   * @param buffer bytes to print
   * @param buffer_size size of bytes to print
   * @return
   */
  void debug_mavlink_msg_buffer(uint8_t *buffer, int buffer_size);

  /**
   * read mavlink message from autopilot using serial port , and push mavlink message to message queue.
   *
   * @return
   */
  void autopilot_read_message();

  /**
   * pop mavlink message from message queue, and send mavlink message to GCS using Dim Socket.
   *
   * @return
   */
  int gcs_write_message();

  /**
   * read mavlink message from GCS using Dim Socket, and push mavlink message to message queue.
   *
   * @return
   */
  int gcs_read_message();

   /**
   * pop mavlink message from message queue, and send mavlink message to autopilot using serial port.
   *
   * @return
   */
  void autopilot_write_message();

  /**
   * main loop for reading mavlink message from autopilot.
   *
   * @return
   */
  void *start_autopilot_read_thread(void *args);

  /**
   * main loop for writing mavlink message to GCS.
   *
   * @return
   */
  void *start_gcs_write_thread(void *args);

  /**
   * main loop for reading mavlink message from GCS.
   *
   * @return
   */
  void *start_gcs_read_thread(void *args);

  /**
   * main loop for writing mavlink message to autopilot.
   *
   * @return
   */
  void *start_autopilot_write_thread(void *args);

  /**
   * exit MAVLinkTlsServer
   * close serial port and DIM TLS Socket.
   * @return
   */
  void exit(int signum);

  /**
   * thread running status.
   * 1 if thread is running, otherwise 0
   */
  sig_atomic_t run;

 private:
  moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_gcs;
  moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_autopilot;

  Serial_Port *port;
  DimSocket *dim;

  volatile int autopilot_writing_status = 0;
  volatile int dim_writing_status = 0;
};

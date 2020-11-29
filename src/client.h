/**
 * @file client.h
 *
 * @brief DIM TLS Client
 *
 * TCP TLS client for communication MAVLink endpoints in GCS
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

#include <mavlink.h>
#include <sys/time.h>

#define BUFFER_LENGTH 8192

#include "readerwriterqueue.h"

/**
 * \class MAVLinkTlsClient
 * \brief MAVLink TLS Client for GCS. It connect with Autopilot and GCS using DIM TLS Socket.
 */
class MAVLinkTlsClient
{

 public:
   /**
   * MAVLinkTlsClient Class constructor.
   *
   * @return
   */
  explicit MAVLinkTlsClient(int _sock, DimSocket *_dim) {
    sock = _sock;
    dim = _dim;

    // recv UDP socket
    memset(&locAddr, 0, sizeof(locAddr));
    locAddr.sin_family = AF_INET;
    locAddr.sin_addr.s_addr = INADDR_ANY;
    locAddr.sin_port = htons(14551);

    /* Bind the socket to port 14551 - necessary to receive packets from GCS like a QGC */
    if (-1 == bind(sock, (struct sockaddr *)&locAddr, sizeof(struct sockaddr))) {
      std::cout << "bind fail " << std::endl;
      close(sock);
      return;
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK | O_ASYNC) < 0)
    {
      memset(&gcAddr, 0, sizeof(gcAddr));
    }

    // send UDP socket
    char target_ip[100];
    strcpy(target_ip, "127.0.0.1");

    gcAddr.sin_family = AF_INET;
    gcAddr.sin_addr.s_addr = inet_addr(target_ip);
    gcAddr.sin_port = htons(14550);

    run = 1;
  }

  /**
   * MAVLinkTlsClient Class deconstructor.
   *
   * @return
   */
  ~MAVLinkTlsClient() {
  }

  /**
   * print mavlink bytes for debugging
   * @param buffer bytes to print
   * @param buffer_size size of bytes to print
   * @return
   */
  void debug_mavlink_msg_buffer(uint8_t *buffer, int buffer_size);

  /**
   * read mavlink message from GCS, and push mavlink message to message queue.
   *
   * @return
   */
  void gcs_read_message();

  /**
   * pop mavlink message from message queue, and send mavlink message to autopilot using DIM Socket.
   *
   * @return
   */
  int autopilot_write_message();

  /**
   * receive mavlink message from autopilot using DIM TLS Socket, and push mavlink message to message queue.
   *
   * @return
   */
  int autopilot_read_message();

  /**
   * pop mavlink message from message queue, and send mavlink message to GCS using UDP Socket.
   *
   * @return
   */
  void gcs_write_message();

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
   * main loop for reading mavlink from autopilot and writing mavlink message to GCS.
   *
   * @return
   */
  void *start_autopilot_read_write_thread(void *args);

  /**
   * exit MAVLinkTlsClient
   * close UDP and DIM TLS Socket.
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

  DimSocket *dim;

  volatile int dim_writing_status = 0;

  int sock;

  struct sockaddr_in gcAddr;
  struct sockaddr_in locAddr;
};

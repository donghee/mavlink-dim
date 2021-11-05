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

#include <dim_client.h>
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
    explicit MAVLinkTlsClient(DimClient *_dim, const char* mavlink_out_ip, int mavlink_out_port) {
    dim = _dim;

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
      fprintf(stderr, "MAVLink socket failed\n");
      ::exit(EXIT_FAILURE);
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK | O_ASYNC) < 0) {
      fprintf(stderr, "MAVLink socket set NONBLOCK flag failed\n");
    }

    memset(&gcAddr, 0, sizeof(gcAddr));
    gcAddr.sin_family = AF_INET;
    gcAddr.sin_addr.s_addr = inet_addr(mavlink_out_ip);
    gcAddr.sin_port = htons(mavlink_out_port);

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

  DimClient *dim;

  volatile int dim_writing_status = 0;

  int sock;

  struct sockaddr_in gcAddr;
    //struct sockaddr_in locAddr;
};

#include "client.h"

MAVLinkTlsClient *g_client;

static void signal_exit(int signum)
{
  std::cout << "Caught signal " << signum << std::endl;
  g_client->exit(signum);
}

void
MAVLinkTlsClient::exit(int signum)
{
  run = 0;

  dim->close();
  dim->power_off();
  ::exit(1);
}

void
MAVLinkTlsClient::debug_mavlink_msg_buffer(uint8_t *buffer, int buffer_size)
{
  printf("MAVLink Buffer: 0x");

  for (int i = 0; i < buffer_size; ++i) {
    printf("%X", buffer[i]);
  }

  printf("\n");

  return;
}

// gcs -> autopilot
void
MAVLinkTlsClient::gcs_read_message()
{
  uint8_t recv_buffer[BUFFER_LENGTH];
  int16_t recv_size;
  socklen_t fromlen = sizeof(gcAddr);

  mavlink_message_t message;
  mavlink_status_t status;

  bool received = false;

  while (!received) {
    recv_size = recvfrom(sock, (void *)recv_buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&gcAddr, &fromlen);

    if (recv_size > 0) {
      for (int i = 0; i < recv_size; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, recv_buffer[i], &message, &status)) {
          q_to_autopilot.enqueue(message);
          received = true;
        }
      }
    }

    usleep(10);
  }
}


int
MAVLinkTlsClient::autopilot_write_message()
{
  int result = 0;
  uint8_t send_buffer[BUFFER_LENGTH];
  int16_t send_size;

  mavlink_message_t message;

  // Fully-blocking
  // q_to_autopilot.wait_dequeue(message);
  // if (q_to_autopilot.wait_dequeue_timed(message, std::chrono::milliseconds(1)))
  if (q_to_autopilot.wait_dequeue_timed(message, std::chrono::microseconds(10))) {
    printf("Received message from gcs with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);

    send_size = mavlink_msg_to_send_buffer((uint8_t *)send_buffer, &message);
    // debug
    debug_mavlink_msg_buffer(send_buffer, send_size);

    if (dim->is_connected()) {
      dim_writing_status = true;
      result = dim->send(send_size, send_buffer);
      dim_writing_status = false;

      if (result == -1) {
        return result;
      }
    }
  }

  return 0;
}

// autopilot -> qgc
int
MAVLinkTlsClient::autopilot_read_message()
{
  int result;
  uint8_t recv_buffer[BUFFER_LENGTH];
  int16_t recv_size;

  mavlink_message_t message;
  mavlink_status_t status;

  bool received = false;

  while (!received && dim->is_connected()) {
    result = dim->recv(&recv_size, recv_buffer);

    if (result > 0) {
      for (int i = 0; i < recv_size; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, recv_buffer[i], &message, &status)) {
          q_to_gcs.enqueue(message);
          received = true;
        }
      }

    } else if (result <= 0) {
      // printf("\nclient result %d, recv_size %d\n", result, recv_size);
      return result;
    }

    usleep(100); //10khz

    if (dim_writing_status > false) {
      usleep(100); // 10kHz
    }
  }

  return 1;
}



void
MAVLinkTlsClient::gcs_write_message()
{
  uint8_t send_buffer[BUFFER_LENGTH];
  int16_t send_size;

  mavlink_message_t message;

  // Fully-blocking
  q_to_gcs.wait_dequeue(message);
  // if (q_to_gcs.wait_dequeue_timed(message, std::chrono::milliseconds(5)))
  {
    printf("Received message from fc with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);
    send_size = mavlink_msg_to_send_buffer((uint8_t *)send_buffer, &message);
    // debug
    // debug_mavlink_msg_buffer(send_buffer, send_size);
    sendto(sock, send_buffer, send_size, 0, (struct sockaddr *)&gcAddr, sizeof(struct sockaddr_in));
  }
}

// read and write
void *
MAVLinkTlsClient::start_gcs_read_thread(void *args)
{
  while (run) {
    gcs_read_message();
    usleep(1000); // 1000hz
  }

  printf("\r\nExit gcs read thread\r\n");
  return NULL;
}

void *
MAVLinkTlsClient::start_autopilot_write_thread(void *args)
{
  int result = 0;

  while (run) {
    while (1) {
      result = autopilot_write_message();

      if (result < 0) {
        printf("\r\nSEND ERROR: %X\r\n", result);
        result = dim->close();
        break;
      }

      // usleep(1000000); // 1hz
      usleep(100000); // 10hz
    }
  }

  printf("\r\nExit autopilot write thread\r\n");
  return NULL;
}

// read and write
void *
MAVLinkTlsClient::start_autopilot_read_thread(void *args)
{
  int result = 0;

  while (run) {
    dim->init_poll();

    while (1) {
      result = autopilot_read_message();

      if (result < 0) {
        if (result == -0x7880) {
          printf("\r\nGot KSETLS_ERROR_TLS_PEER_CLOSE_NOTIFY\r\n");
          continue;

        } else if (result == -0x7780) {
          printf("\r\nGot KSETLS_ERROR_TLS_FATAL_ALERT_MESSAGE\r\n");

        } else if (result == -1) {
          printf("\r\nREAD ERROR: %X\r\n", result);
          printf("\r\nTry dim->connect\r\n");
          // sleep(1);
          result = dim->close();
          dim->connect();
          printf("\r\nContinue to read message\r\n");
          break;

        } else {
          printf("\r\nREAD ERROR: %X\r\n", result);
        }

        sleep(1);
        result = dim->close();
        dim->connect();
        break;
      }

      if (result == 0) {
        // printf("READ ERROR: DIM TLS CLOSE NOTIFY\r\n");
        // result = dim->tls_close_notify();
        // printf("\r\nREAD ERROR: DIM TLS CLOSE NOTIFY: %d\r\n", result);
        continue;
      }

      //usleep(100); // 10000hz
      usleep(10); // 10000hz
    }
  }

  printf("\r\nExit autopilot read thread\r\n");
  return NULL;
}

void *
MAVLinkTlsClient::start_gcs_write_thread(void *args)
{
  while (run) {
    gcs_write_message();
    usleep(1000); // 1000hz
  }

  printf("\r\nExit gcs write thread\r\n");
  return NULL;
}

void *
MAVLinkTlsClient::start_autopilot_read_write_thread(void *args)
{
  struct pollfd *fds;
  int result = 0;

  while (run) {
    dim->init_poll();

    while (1) {
      fds = dim->poll_descriptor();

      if (poll(fds, 1, DIM_TIMEOUT) > 0) {
        if (fds[0].revents & (POLLNVAL | POLLERR | POLLHUP)) {
          printf("\r\n POLL ERRORS\r\n");
          result = dim->close();
          dim->connect();
          break;
        }

        if (fds[0].revents & POLLIN) {
          result = autopilot_read_message();

          if (result < 0) {
            if (result == -0x7880) {
              printf("\r\nGot KSETLS_ERROR_TLS_PEER_CLOSE_NOTIFY\r\n");
              result = dim->close();
              dim->connect();
              break;

            } else if (result == -0x7780) {
              printf("\r\nGot KSETLS_ERROR_TLS_FATAL_ALERT_MESSAGE\r\n");

            } else if (result == -1) {
              printf("\r\nREAD ERROR: %X\r\n", result);
              printf("\r\nTry dim->connect\r\n");
              // sleep(1);
              result = dim->close();
              dim->connect();
              printf("\r\nContinue to read message\r\n");
              break;

            } else {
              printf("\r\nREAD ERROR: %X\r\n", result);
              sleep(1);
              result = dim->close();
              dim->connect();
              break;
            }
          }
        }

        // if(fds[0].revents & POLLOUT) {
        result = autopilot_write_message();

        if (result < 0) {
          printf("\r\nSEND ERROR: %X\r\n", result);
          result = dim->close();
          break;
        }

        usleep(10);
        // }
      }
    }
  }

  return NULL;
}

typedef void * (*THREADFUNCPTR)(void *);

int
start_client_threads(int _sock, DimSocket *dim) {
  // pthread
  int result;
  pthread_t autopilot_read_tid, gcs_write_tid, gcs_read_tid, autopilot_write_tid;
  pthread_t autopilot_read_write_tid;

  MAVLinkTlsClient *client;
  client = new MAVLinkTlsClient(_sock, dim);
  g_client = client;

  signal(SIGINT, signal_exit);

  // result = pthread_create( &autopilot_read_tid, NULL, &start_autopilot_read_thread, (char*)"Autopilot Reading" );
  // if ( result ) throw result;

  result = pthread_create(&gcs_write_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsClient::start_gcs_write_thread, client);

  if (result) { throw result; }

  result = pthread_create(&gcs_read_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsClient::start_gcs_read_thread, client);

  if (result) { throw result; }

  // result = pthread_create( &autopilot_write_tid, NULL, &start_autopilot_write_thread, (char*)"Autopilot Writing" );
  // if ( result ) throw result;

  result = pthread_create(&autopilot_read_write_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsClient::start_autopilot_read_write_thread, client);

  if (result) { throw result; }

  // wait for exit
  // pthread_join(autopilot_read_tid, NULL);
  pthread_join(gcs_write_tid, NULL);
  pthread_join(gcs_read_tid, NULL);
  // pthread_join(autopilot_write_tid, NULL);
  pthread_join(autopilot_read_write_tid, NULL);

  return NULL;
}

int main(int argc, const char *argv[])
{
  int sock;
  DimSocket *dim;

  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (argc !=2) {
    fprintf(stderr, "Usage: %s 10.243.45.201 \n", argv[0]);
    exit(EXIT_FAILURE);
  }

  dim = new DimSocket(argv[1], 4433);

  start_client_threads(sock, dim);

  dim->close();

  return EXIT_SUCCESS;
}

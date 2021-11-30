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

  while (!received && run) {
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

      if (result < 0) {
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
  // q_to_gcs.wait_dequeue(message);
  if (q_to_gcs.wait_dequeue_timed(message, std::chrono::milliseconds(5))) {
    printf("Received message from fc with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);
    send_size = mavlink_msg_to_send_buffer((uint8_t *)send_buffer, &message);
    // debug
    debug_mavlink_msg_buffer(send_buffer, send_size);
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

  // dim->init_poll();

  while (run) {
    dim->init_poll();

    while (1 && run) {
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
          dim->connect();
          break;
        }

        usleep(10);
        // }
      }
    }
  }

  printf("\r\nExit autopilot read write thread\r\n");

  return NULL;
}

typedef void *(*THREADFUNCPTR)(void *);

pthread_t autopilot_read_write_tid, gcs_write_tid, gcs_read_tid;

int
start_client_threads(MAVLinkTlsClient *client)
{
  int result;

  result = pthread_create(&gcs_write_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsClient::start_gcs_write_thread, client);

  if (result) { return result; }

  result = pthread_create(&gcs_read_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsClient::start_gcs_read_thread, client);

  if (result) { return result; }

  result = pthread_create(&autopilot_read_write_tid, NULL,
                          (THREADFUNCPTR) &MAVLinkTlsClient::start_autopilot_read_write_thread, client);

  if (result) { return result; }

  // wait for exit
  // pthread_join(gcs_write_tid, NULL);
  // pthread_join(gcs_read_tid, NULL);
  // pthread_join(autopilot_read_write_tid, NULL);

  return NULL;
}

int
wait_client_threads()
{
  pthread_join(gcs_write_tid, NULL);
  pthread_join(gcs_read_tid, NULL);
  pthread_join(autopilot_read_write_tid, NULL);

  return NULL;
}


int init_commander(int &commander_sock, int port)
{
  struct sockaddr_in commanderAddr;

  commander_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  memset(&commanderAddr, 0, sizeof(commanderAddr));
  commanderAddr.sin_family = AF_INET;
  commanderAddr.sin_addr.s_addr = INADDR_ANY;
  commanderAddr.sin_port = htons(port);

  if (-1 == bind(commander_sock, (struct sockaddr *)&commanderAddr, sizeof(struct sockaddr))) {
    std::cout << "bind fail " << std::endl;
    close(commander_sock);
    return -1;
  }

  return 0;
}

int main(int argc, const char *argv[])
{
  int commander_sock;
  int mavlink_out_address;
  int mavlink_out_port;
  int commander_in_port;

  char dim_tls_server_ip[16];

  if (argc < 4) {
    fprintf(stderr, "Usage: %s 10.243.45.201 <mavlink_out_port> <command_listen_port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  strcpy(dim_tls_server_ip, argv[1]);
  mavlink_out_port = atoi(argv[2]);
  commander_in_port = atoi(argv[3]);

  printf("Running Client <Server IP: %s, MAVLink PORT: %u, Commander PORT: %u>\n", dim_tls_server_ip, mavlink_out_port,
         commander_in_port);

  init_commander(commander_sock, commander_in_port);
  struct sockaddr_in commanderClientAddr;
  socklen_t commanderClientAddrLen = sizeof(struct sockaddr_in);

  DimClient *dim = new DimClient();
  MAVLinkTlsClient *client = new MAVLinkTlsClient(dim, "127.0.0.1\0", mavlink_out_port);

  g_client = client;
  signal(SIGINT, signal_exit);

  // start_client_threads(client);
  // wait_client_threads();

  // while 1
  // read command
  // if connect command: stop thread (run = 0) and start thread (run = 1)
  // if disconnect command: stop thread (run = 0)
  // if auth command with id, pass: send server and read key
  // if encrypt command with plaintext:
  // if decrypt command with cypertext:

  char commander_buffer[512];
  int received = 0;

  while (1) {
    sleep(1);

    if ((received = recvfrom(commander_sock, commander_buffer, 512, 0, (struct sockaddr *)&commanderClientAddr,
                             &commanderClientAddrLen)) != -1) {
      if (received > 4) {
        printf("Received command: %s", commander_buffer);
        char *command = new char[4]();
        memcpy(command, &commander_buffer[0], 4);

        if (strncmp(command, "start", 4) == 0) { // connect
          client->run = 0;
          wait_client_threads();
          client->run = 1;

          dim->open(dim_tls_server_ip, 4433);
          start_client_threads(client);
        }

        if (strncmp(command, "stop", 4) == 0) { // disconnect
          client->run = 0;
          wait_client_threads();

          dim->close();
        }

        if (strncmp(command, "auth", 4) == 0) { // auth
          client->run = 0;
          wait_client_threads();

          char *auth_key = new char[received - 5]();
          memcpy(auth_key, &commander_buffer[5], received - 5);
          printf("auth key: %s", auth_key);
        }

        if (strncmp(command, "decrypt", 4) == 0) { // decrypt
          client->run = 0;
          wait_client_threads();

          printf("\r\ncommander_buffer: received: %d\r\n", received);
          uint8_t buffer [256 + 16 + 128 + 16];
          uint8_t plaintext [256];
          memcpy(buffer, &commander_buffer[8], received - 8);

          printf("dim->decrypt() \r\n");
          dim->decrypt(1, 1, (const char *)buffer, 256 + 16 + 128 + 16, plaintext);

          sendto(commander_sock, plaintext, 256, 0, (struct sockaddr *)&commanderClientAddr, commanderClientAddrLen);
        }

        if (strncmp(command, "random", 4) == 0) { // random
          client->run = 0;
          wait_client_threads();

          uint8_t buffer [16];
          dim->generate_random(buffer, 16);
          sendto(commander_sock, buffer, 16, 0, (struct sockaddr *)&commanderClientAddr, commanderClientAddrLen);
        }

        if (strncmp(command, "encrypt", 4) == 0) { // encrypt
          client->run = 0;
          wait_client_threads();

          char *plaintext = new char[received - 8]();
          memcpy(plaintext, &commander_buffer[8], received - 8);

          uint8_t abPubKey0[64] = {0x03, 0x10, 0x81, 0x8A, 0x36, 0xE2, 0xCB, 0x32, 0x0A, 0xFD, 0x92, 0xEC, 0xE3, 0x52, 0x3D, 0x1A};

          dim->set_key((const uint8_t *)abPubKey0, sizeof(abPubKey0) / sizeof(uint8_t));

          int i;
          int16_t sRv;
          uint8_t abData[256], abTag[16];
          uint8_t abData1[256], abData2[256];
          uint8_t abIv[16], abAuth[128];

          // IV
          sRv = dim->generate_random(abIv, 16);

          if (sRv != 0) {
            printf("IV : Fail(-0x%04X)\r\n", -sRv);
          }

          // Auth
          sRv = dim->generate_random(abAuth, 128);

          if (sRv != 0) {
            printf("Auth : Fail(-0x%04X)\r\n", -sRv);
          }

          // Plaintext
          printf("  * Plain Data :\r\n    ");

          for (i = 0; i < 256; i++) {
            abData[i] = (uint8_t)plaintext[i];
            //abData[i]= 0xaa;
            printf("%02X", abData[i]);

            if ((i < 255) && ((i + 1) % 32 == 0)) {
              printf("\r\n    ");
            }
          }

          printf("\r\ndim->encrypt() \r\n");
          dim->encrypt(abData, 256, abIv, 16, abAuth, 128, abData1, abTag, 16);

          uint8_t result [256 + 16 + 128 + 16];
          memcpy(result, &abData1[0], 256);
          memcpy(result + 256, &abIv[0], 16);
          memcpy(result + 256 + 16, &abAuth[0], 128);
          memcpy(result + 256 + 16 + 128, &abTag[0], 16);

          printf("  * Result Data :\r\n    ");

          for (i = 0; i < 256 + 16 + 128 + 16; i++) {
            printf("%02X", result[i]);

            if ((i < 512) && ((i + 1) % 32 == 0)) {
              printf("\r\n    ");
            }
          }

          // send UDP socket
          sendto(commander_sock, result, 256 + 16 + 128 + 16, 0, (struct sockaddr *)&commanderClientAddr, commanderClientAddrLen);
        }
      }
    }
  }

  dim->close();

  return EXIT_SUCCESS;
}

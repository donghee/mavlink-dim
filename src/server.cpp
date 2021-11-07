#include "server.h"

MAVLinkTlsServer *g_server;

static void signal_exit(int signum)
{
  std::cout << "Caught signal " << signum << std::endl;
  g_server->exit(signum);
}

void
MAVLinkTlsServer::exit(int signum)
{
  run = 0;

  port->stop();
  dim->close();
  dim->power_off();
  ::exit(1);
}

void
MAVLinkTlsServer::debug_mavlink_msg_buffer(uint8_t *buffer, int buffer_size)
{
  printf("MAVLink Buffer: 0x");

  for (int i = 0; i < buffer_size; ++i) {
    printf("%X", buffer[i]);
  }

  printf("\n");

  return;
}

// autopilot -> gcs
void
MAVLinkTlsServer::autopilot_read_message()
{
  int result;
  bool received = false;
  bool received_heartbeat = false;
  bool received_sys_status = false;
  bool received_attitude = false;
  mavlink_message_t message;
  uint8_t encrypted_text[256];

  while (!received) {
    result = port->read_message(message);

    if (result > 0) {
      if (
              //message.msgid == MAVLINK_MSG_ID_ATTITUDE ||
              //message.msgid == MAVLINK_MSG_ID_ALTITUDE ||
              message.msgid == MAVLINK_MSG_ID_ATTITUDE_QUATERNION ||
              message.msgid == MAVLINK_MSG_ID_ATTITUDE_TARGET ||
              message.msgid == MAVLINK_MSG_ID_LOCAL_POSITION_NED ||
              message.msgid == MAVLINK_MSG_ID_SERVO_OUTPUT_RAW ||
              message.msgid == MAVLINK_MSG_ID_HIGHRES_IMU ||
              message.msgid == MAVLINK_MSG_ID_TIMESYNC ||
              message.msgid == MAVLINK_MSG_ID_VFR_HUD ||
              message.msgid == MAVLINK_MSG_ID_VIBRATION ||
              message.msgid == MAVLINK_MSG_ID_ESTIMATOR_STATUS ||
              message.msgid == MAVLINK_MSG_ID_SCALED_IMU ||
              message.msgid == MAVLINK_MSG_ID_SCALED_IMU2 ||
              message.msgid == MAVLINK_MSG_ID_ACTUATOR_CONTROL_TARGET ||
              message.msgid == MAVLINK_MSG_ID_ODOMETRY ||
              message.msgid == MAVLINK_MSG_ID_EXTENDED_SYS_STATE ||
              message.msgid == MAVLINK_MSG_ID_BATTERY_STATUS
          ) {
        continue;
      }

      if (message.msgid == MAVLINK_MSG_ID_ENCAPSULATED_DATA) {
        mavlink_encapsulated_data_t encapsulated_data;
        mavlink_msg_encapsulated_data_decode(&message, &encapsulated_data);

        // printf("encapsulated_data %d\n", encapsulated_data.seqnr);
        if (encapsulated_data.seqnr < 2) { // 0,1
          for(int i = 0 ; i < 128; i++) {
            // printf("%02X", encapsulated_data.data[i]);
            encrypted_text[i+(128*encapsulated_data.seqnr)] = encapsulated_data.data[i];
          }
        } else { // 2
          uint8_t plain_text[256], abIv[16], abAuth[128], abTag[16];

          for(int i = 0 ; i < 16; i++) {
            // printf("%02X", encapsulated_data.data[i]);
            abIv[i] = encapsulated_data.data[i];
            // printf("%02X", abIv[i]);
          }
          // printf("\n");

          for(int i = 0 ; i < 128; i++) {
            // printf("%02X", encapsulated_data.data[i]);
            abAuth[i] = encapsulated_data.data[16+i];
            // printf("%02X", abAuth[i]);
          }
          // printf("\n");

          for(int i = 0 ; i < 16; i++) {
            // printf("%02X", encapsulated_data.data[i]);
            abTag[i] = encapsulated_data.data[16+128+i];
            // printf("%02X", abTag[i]);
          }
          printf("\n");
          printf("  * Received Encrypted Data using MAVLink from FC \r\n    ");

          for (int i = 0; i < 256; i++) {
            printf("%02X", encrypted_text[i]);

            if ((i < 255) && ((i + 1) % 32 == 0)) {
              printf("\r\n    ");
            }
          }
          printf("\r\n");
          printf("  * Decrypted Plain: MAVLink #33 message from FC \r\n    ");
          // ARIA (0x50) Decrypt
          memset(&plain_text, 0, sizeof(plain_text));
          int ret;
          ret = _kcmvpAriaGcm(plain_text, encrypted_text, 256, KCMVP_ARIA128_KEY, 0, abIv, 16,abAuth, 128, abTag, 16, DECRYPT);
          // printf("kcmvpAriaGcm: %d    \n", ret);
          // printf("    ");
          for (int i = 0; i < 256; i++) {
            printf("%02X", plain_text[i]);

            if ((i < 255) && ((i + 1) % 32 == 0)) {
              printf("\r\n    ");
            }
          }
          printf("\n");
          // get key 0
          // uint8_t abPubKey0[64];
          // uint16_t usSize0 = 0;
          // memset(&abPubKey0, 0, sizeof(abPubKey0));
          // ret = _kcmvpGetKey(abPubKey0, &usSize0, KCMVP_ARIA128_KEY, 0);
          // if (ret < 0 ) {
          //   printf("Error kcmvpGetKey: %d\n", ret);
          // }
          // printf("kcmvpGetKey Size: %d\n", usSize0);
          // printf("kcmvpGetKey: \n");
          // for (int i = 0; i < usSize0; i++) {
          //   printf("%02X", abPubKey0[i]);
          // }

          // printf("\r\n");

          // set key 0
          // abPubKey0[0] = 0x03;
          // abPubKey0[1] = 0x10;
          // abPubKey0[2] = 0x81;
          // abPubKey0[3] = 0x8A;
          // abPubKey0[4] = 0x36;
          // abPubKey0[5] = 0xE2;
          // abPubKey0[6] = 0xCB;
          // abPubKey0[7] = 0x32;
          // abPubKey0[8] = 0x0A;
          // abPubKey0[9] = 0xFD;
          // abPubKey0[10] = 0x92;
          // abPubKey0[11] = 0xEC;
          // abPubKey0[12] = 0xE3;
          // abPubKey0[13] = 0x52;
          // abPubKey0[14] = 0x3D;
          // abPubKey0[15] = 0x1A;

          // ret = _kcmvpEraseKey(KCMVP_ARIA128_KEY, 0);
          // if (ret < 0 ) {
          //   printf("Error kcmvpEraseKey: %d\n", ret);
          // }

          // ret = _kcmvpPutKey(KCMVP_ARIA128_KEY, 0, abPubKey0, 16);
          // if (ret < 0 ) {
          //   // printf("Error kcmvpPutKey: %d\n", ret);
          //   printf("Error kcmvpGenerateKey: %d\n", ret);
          // }

          // ret = _kcmvpGenerateKey(KCMVP_ARIA128_KEY, 0, 0);
          // if (ret < 0 ) {
          //   // printf("Error kcmvpPutKey: %d\n", ret);
          //   printf("Error kcmvpGenerateKey: %d\n", ret);
          // }
        }

        continue;
      }

      q_to_gcs.enqueue(message);

      if (message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
        received_heartbeat = true;
      }

      received = received_heartbeat;
    }

    usleep(10); // 10kHz

    if (autopilot_writing_status > false) {
      usleep(100);    // 10kHz
    }
  }
}

int
MAVLinkTlsServer::gcs_write_message()
{
  int result = 0;
  uint8_t send_buffer[BUFFER_LENGTH];
  int16_t recv_size;

  mavlink_message_t message;
  mavlink_sys_status_t sys_status;
  // mavlink_auth_key_t auth_key;

  memset(send_buffer, 0, BUFFER_LENGTH);

  // Fully-blocking
  // q_to_gcs.wait_dequeue(message);
  if (q_to_gcs.wait_dequeue_timed(message, std::chrono::milliseconds(1))) {
    /*
    // Modify sys status
    if (message.msgid == MAVLINK_MSG_ID_SYS_STATUS) {
    mavlink_msg_sys_status_decode(&message, &sys_status);
    sys_status.errors_count4 = 1;
    mavlink_msg_sys_status_encode(1, 1, &message, &sys_status);
    // memset(auth_key.key, '\0', 32);
    // mavlink_msg_auth_key_encode(1, 1, &message, &auth_key);
    }
    */

    // printf(" Received message from serial with ID #%d (sys:%d|comp:%d):\r\n", message.msgid,
    // message.sysid, message.compid);
    recv_size = mavlink_msg_to_send_buffer((uint8_t *)send_buffer, &message);

    // debug
    //debug_mavlink_msg_buffer(send_buffer, recv_size);
    if (dim->is_connected()) {
      dim_writing_status = true;
      result = dim->send(recv_size, send_buffer);
      dim_writing_status = false;

      if (result < 0) {
        sleep(1);
        return result;
      }
    }
  }

  return 0;
}

// gcs -> autopilot
int
MAVLinkTlsServer::gcs_read_message()
{
  int result = 0;
  bool received = false;
  mavlink_message_t message;
  mavlink_status_t status;

  uint8_t recv_buffer[BUFFER_LENGTH];
  int16_t recv_size;

  memset(recv_buffer, 0, BUFFER_LENGTH);

  while (!received && dim->is_connected()) {
    result = dim->recv(&recv_size, recv_buffer);

    if (result > 0) {
      for (int i = 0; i < recv_size; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, recv_buffer[i], &message, &status)) {
          q_to_autopilot.enqueue(message);
          received = true;
        }
      }

    } else if (result <= 0) {
      // printf("\nserver result %d, recv_size %d\n", result, recv_size);
      return result;
    }

    // else if (result == 0 && recv_size != 0) {
    //     printf("\nserver result %d, recv_size %d\n", result, recv_size);
    //     return result;
    // }
    usleep(10); // 10kHz

    if (dim_writing_status > false) {
      usleep(100);    // 10kHz
    }
  }

  return 1;
}

void
MAVLinkTlsServer::autopilot_write_message()
{
  uint8_t recv_buffer[BUFFER_LENGTH];
  int16_t recv_size;

  mavlink_message_t message;

  memset(recv_buffer, 0, BUFFER_LENGTH);

  // Fully-blocking
  // q_to_autopilot.wait_dequeue(message);
  if (q_to_autopilot.wait_dequeue_timed(message, std::chrono::milliseconds(1))) {
    printf("Received message from gcs with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);
    // debug
    //recv_size = mavlink_msg_to_send_buffer((uint8_t*)recv_buffer, &message);
    //debug_mavlink_msg_buffer(recv_buffer, recv_size);

    autopilot_writing_status = true;

    if (port) {
      port->write_message(message);
    }

    autopilot_writing_status = false;
  }
}

void *
MAVLinkTlsServer::start_autopilot_read_thread(void *args)
{
  //while (run) {
  while (1) {
    autopilot_read_message();
    usleep(10); // 10khz
  }

  printf("\r\nExit autopilot read thread\r\n");
  return NULL;
}

void *
MAVLinkTlsServer::start_gcs_write_thread(void *args)
{
  int result;

  while (run) {
    result = gcs_write_message();

    if (result < 0) {
      printf("\r\nSEND ERROR: %X\r\n", result);
    }

    usleep(10); // 10khz
  }

  printf("\r\nExit gcs write thread\r\n");
  return NULL;
}

void *
MAVLinkTlsServer::start_gcs_read_thread(void *args)
{
  int result;
  int no_data_count = 0; // no data counts

  while (run) {
    if (dim->accept() < 0) {
      // dim->close();
      printf("\r\nACCEPT failed\r\n");
      continue;
    }

    dim->init_poll();
    printf("\r\nStart gcs read message\r\n");

    while (1) {
      result = gcs_read_message();

      if (result < 0) { // ERROR
        if (result == -0x7880) {
          printf("\r\nGot KSETLS_ERROR_TLS_PEER_CLOSE_NOTIFY\r\n");
          continue;

        } else if (result == -0x7780) {
          printf("\r\nGot KSETLS_ERROR_TLS_FATAL_ALERT_MESSAGE\r\n");
          usleep(100000); // 0.1s
          result = dim->tls_close_notify();
          //printf("\r\nresult: %d\r\n", result);
          //if (result == 0)
		  //dim->tls_close();
          //usleep(500000); // 0.5s
          dim->close();
          break;

        } else {
          printf("\r\nDIM READ ERROR: %X\r\n", result);
        }

        break;
      }

      if (result == 0) { // NO DATA
        no_data_count += 1;

        //if (no_data_count > 10000) {
        //if (no_data_count > 1000) {
        if (no_data_count > 5000) {
          // printf("Timeout: %d\r\n", no_data_count);
          // sleep(1);
          usleep(100000); // 0.1s
          result = dim->tls_close_notify();
          dim->close();
          no_data_count = 0;
          break;
        }

      } else { // ON DATA
        //printf("Timeout: %d\r\n", no_data_count);
        no_data_count = 0;
      }

      usleep(10);
    }
  }

  printf("\r\nExit gcs read thread\r\n");
  return NULL;
}

void *
MAVLinkTlsServer::start_autopilot_write_thread(void *args)
{
  while (run) {
    autopilot_write_message();
    usleep(1000); // 1000hz
    //usleep(100000); // 10hz
  }

  printf("\r\nExit autopilot write thread\r\n");
  return NULL;
}

typedef void * (*THREADFUNCPTR)(void *);

pthread_t autopilot_read_tid, gcs_read_tid, gcs_write_tid, autopilot_write_tid;

int
start_server_threads(MAVLinkTlsServer *server)
{
  int result;

  //result = pthread_create(&autopilot_read_tid, NULL, &start_autopilot_read_thread, (char *)"Autopilot Reading");
  result = pthread_create(&autopilot_read_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsServer::start_autopilot_read_thread, server);

  if (result) { return result; }

  result = pthread_create(&gcs_write_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsServer::start_gcs_write_thread, server);

  if (result) { return result; }

  result = pthread_create(&gcs_read_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsServer::start_gcs_read_thread, server);

  if (result) { return result; }

  result = pthread_create(&autopilot_write_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsServer::start_autopilot_write_thread, server);

  if (result) { return result; }

  return NULL;
}

int
wait_server_threads() {
  pthread_join(gcs_write_tid, NULL);
  pthread_join(gcs_read_tid, NULL);
  pthread_join(autopilot_write_tid, NULL);
  pthread_join(autopilot_read_tid, NULL);

  return NULL;
}

int init_commander(int& commander_sock) {
  struct sockaddr_in commanderAddr;

  commander_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  memset(&commanderAddr, 0, sizeof(commanderAddr));
  commanderAddr.sin_family = AF_INET;
  commanderAddr.sin_addr.s_addr = INADDR_ANY;
  commanderAddr.sin_port = htons(9120);

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
  init_commander(commander_sock);
  struct sockaddr_in commanderClientAddr;
  socklen_t commanderClientAddrLen = sizeof(struct sockaddr_in);

  char commander_buffer[512];
  int received = 0;
  bool is_authentication = false;

  if (argc !=3) {
    fprintf(stderr, "Usage: %s /dev/ttyTHS1 921600\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  DimServer *dim;
  Serial_Port *port;
  MAVLinkTlsServer *server;

  port = new Serial_Port(argv[1], atoi(argv[2]));
  dim = new DimServer("0.0.0.0", 4433);
  server = new MAVLinkTlsServer(port, dim);
  g_server = server;

  signal(SIGINT, signal_exit);

  /*
    port->start();
    start_server_threads(server);
    wait_server_threads();
  */

  while (1) {
    sleep(1);
    if ((received = recvfrom(commander_sock, commander_buffer, 512, 0, (struct sockaddr *)&commanderClientAddr, &commanderClientAddrLen)) != -1) {
      if (received > 4) {
        printf("Received command: %s\n", commander_buffer);
        char * command = new char[4]();
        memcpy(command, &commander_buffer[0], 4);

        // receive encrypted data from fc with ENCAPSULATED DATA message
        if (strncmp(command, "receive ", 4) == 0) {
          server->run = 0;
          wait_server_threads();
          server->run = 1;

          port->start();

          mavlink_message_t message;
          uint8_t encrypted_text[256];
          // mavlink_encapsulated_data_key_t encapsulated_data;
          // memset(encapsulated_data.key, '\0', 32);
          // memcpy(encapsulated_data.key, &commander_buffer[5], received - 5);
          // mavlink_msg_encapsulated_data_encode(1, 1, &message, &encapsulated_data);

          if (port) {
            //port->write_message(message);
            usleep(100);
            while(1) {
              if (port->read_message(message) > 0) {
                if (message.msgid == MAVLINK_MSG_ID_ENCAPSULATED_DATA) {
                  mavlink_encapsulated_data_t encapsulated_data;
                  mavlink_msg_encapsulated_data_decode(&message, &encapsulated_data);

                  if (encapsulated_data.seqnr < 2) { // 0,1
                    for(int i = 0 ; i < 128; i++) {
                      encrypted_text[i+(128*encapsulated_data.seqnr)] = encapsulated_data.data[i];
                    }
                    // memcpy(&encrypted_text[128*encapsulated_data.seqnr], &encapsulated_data.data[0], 128);
                  } else { // 2
                    uint8_t plain_text[256], abIv[16], abAuth[128], abTag[16];

                    for(int i = 0 ; i < 16; i++) {
                      abIv[i] = encapsulated_data.data[i];
                    }

                    for(int i = 0 ; i < 128; i++) {
                      abAuth[i] = encapsulated_data.data[16+i];
                    }

                    for(int i = 0 ; i < 16; i++) {
                      abTag[i] = encapsulated_data.data[16+128+i];
                    }
                    printf("\n");
                    printf("  * Received Encrypted Data using MAVLink from FC \r\n    ");
                    for (int i = 0; i < 256; i++) {
                      printf("%02X", encrypted_text[i]);
                      if ((i < 255) && ((i + 1) % 32 == 0)) {
                        printf("\r\n    ");
                      }
                    }
                    printf("\r\n");
                    sendto(commander_sock, encrypted_text, 256, 0, (struct sockaddr *)&commanderClientAddr, commanderClientAddrLen);
                    break;
                  }
                }
              }
            }
          }
        }

        // send encrypted data to fc with ENCAPSULATED DATA message
        if (strncmp(command, "send ", 4) == 0) {
          server->run = 0;
          wait_server_threads();
          server->run = 1;

          port->start();

          mavlink_message_t messages[3];
          uint8_t encrypted_text[512];

          mavlink_encapsulated_data_t send_encapsulated_data;
          memset(send_encapsulated_data.data, '\0', 253);
          send_encapsulated_data.seqnr = 0;
          memcpy(send_encapsulated_data.data, &commander_buffer[5], 128);
          mavlink_msg_encapsulated_data_encode(1, 1, &messages[0], &send_encapsulated_data);

          send_encapsulated_data.seqnr = 1;
          memset(send_encapsulated_data.data, '\0', 253);
          memcpy(send_encapsulated_data.data, &commander_buffer[5+128], 128);
          mavlink_msg_encapsulated_data_encode(1, 1, &messages[1], &send_encapsulated_data);

          send_encapsulated_data.seqnr = 2;
          memset(send_encapsulated_data.data, '\0', 253);
          memcpy(send_encapsulated_data.data, &commander_buffer[5+128+128], 16+128+16);
          mavlink_msg_encapsulated_data_encode(1, 1, &messages[2], &send_encapsulated_data);

          if (port) {
            for (int i = 0 ; i < 3; i++) {
              printf("send message to fc %d\n", i);
              port->write_message(messages[i]);
              sleep(1);
            }

          }

          port->stop();
          memset(commander_buffer, '\0', 512);
        }

        if (strncmp(command, "auth", 4) == 0) { // connect
          server->run = 0;
          wait_server_threads();
          server->run = 1;

          port->start();

          mavlink_message_t message;
          mavlink_auth_key_t auth_key_msg;
          memset(auth_key_msg.key, '\0', 32);
          memcpy(auth_key_msg.key, &commander_buffer[5], received - 5);
          mavlink_msg_auth_key_encode(1, 1, &message, &auth_key_msg);

          if (port) {
            port->write_message(message);
            while(1) {
              if (port->read_message(message) > 0) {
                if (message.msgid == MAVLINK_MSG_ID_AUTH_KEY) {
                  port->stop();
                  mavlink_auth_key_t ack_auth_key_msg;
                  mavlink_msg_auth_key_decode(&message, &ack_auth_key_msg);
                  printf("AUTH KEY:\n");
                  for(int i = 0 ; i < 16; i++) {
                    printf("%02X", ack_auth_key_msg.key[i]);
                  }
                  printf("\n");
                  if (memcmp(auth_key_msg.key, ack_auth_key_msg.key, 16) == 0) {
                    is_authentication = true;
                    sendto(commander_sock, "Auth key is matched", 19, 0, (struct sockaddr *)&commanderClientAddr, commanderClientAddrLen);
                  } else {
                    is_authentication = false;
                    sendto(commander_sock, "Auth key does not match", 23, 0, (struct sockaddr *)&commanderClientAddr, commanderClientAddrLen);
                  }
                  break;
                }
              }
            }
            continue;
          }
        }

        if (strncmp(command, "start", 4) == 0 && is_authentication) { // start handshake
          port->start();
          start_server_threads(server);

          // not reatched
          wait_server_threads();
          break;
        }
      }
    }
  }

  dim->close();
  port->stop();

  return EXIT_SUCCESS;
}

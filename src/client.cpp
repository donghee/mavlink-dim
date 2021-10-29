#include "client.h"

extern "C" {
#include "kse_ubuntu.h"
}

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
  if (q_to_gcs.wait_dequeue_timed(message, std::chrono::milliseconds(5)))
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

typedef void * (*THREADFUNCPTR)(void *);

pthread_t autopilot_read_write_tid, gcs_write_tid, gcs_read_tid;

int
start_client_threads(MAVLinkTlsClient *client, DimClient *dim) {
  int result;

  result = pthread_create(&gcs_write_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsClient::start_gcs_write_thread, client);

  if (result) { return result; }

  result = pthread_create(&gcs_read_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsClient::start_gcs_read_thread, client);

  if (result) { return result; }

  result = pthread_create(&autopilot_read_write_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsClient::start_autopilot_read_write_thread, client);

  if (result) { return result; }

  // wait for exit
  // pthread_join(gcs_write_tid, NULL);
  // pthread_join(gcs_read_tid, NULL);
  // pthread_join(autopilot_read_write_tid, NULL);

  return NULL;
}

int
wait_client_threads() {
  pthread_join(gcs_write_tid, NULL);
  pthread_join(gcs_read_tid, NULL);
  pthread_join(autopilot_read_write_tid, NULL);

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

int dim_encrypt(uint8_t* abData, int abData_i, uint8_t* abIv, int abIv_i, uint8_t* abAuth, int abAuth_i, uint8_t* abData1, int abData1_i, uint8_t* abTag, int abTag_i) {
    int16_t sRv;
    int i;
    uint16_t usSize0, usSize1;
    //uint8_t abData[256], abData1[256], abData2[256], abIv[16], abAuth[128], abTag[16];
    //uint8_t abData1[256], abData2[256];
    //, abTag[16];

   // Encrypt abData to abData1
    sRv = _kcmvpAriaGcm(abData1, abData, 256, KCMVP_ARIA128_KEY, 0, abIv, 16,
                        abAuth, 128, abTag, 16, ENCRYPT);
    return sRv;
}

int dim_decrypt(int name, int key, const char* buffer, uint8_t* plain_text) {
  //uint8_t plain_text[256];
  uint8_t abIv[16], abAuth[128], abTag[16], encrypted_text[256];

  for(int i = 0 ; i < 16; i++) {
    abIv[i] = buffer[i];
    // printf("%02X", abIv[i]);
  }
  // printf("\n");

  for(int i = 0 ; i < 128; i++) {
    abAuth[i] = buffer[16+i];
    // printf("%02X", abAuth[i]);
  }
  // printf("\n");

  for(int i = 0 ; i < 16; i++) {
    abTag[i] = buffer[16+128+i];
    // printf("%02X", abTag[i]);
  }

  for(int i = 0 ; i < 256; i++) {
    encrypted_text[i] = buffer[i+16+128+16];
  }

  printf("\r\n");
  printf("dim_decrypt()\r\n");
  printf("  * Decrypted Plaintext \r\n    ");

  int ret;

  ret = _kcmvpAriaGcm(plain_text, encrypted_text, 256, KCMVP_ARIA128_KEY, 0, abIv, 16,abAuth, 128, abTag, 16, DECRYPT);
  for (int i = 0; i < 256; i++) {
    printf("%02X", plain_text[i]);

    if ((i < 255) && ((i + 1) % 32 == 0)) {
      printf("\r\n    ");
    }
  }
  printf("\n");
}

//int dim_get_key(char *abPubKey0) {
int dim_get_key() {
    int ret;
    // get key 0
    uint8_t abPubKey0[64];
    uint16_t usSize0 = 0;
    memset(&abPubKey0, 0, sizeof(abPubKey0));
    ret = _kcmvpGetKey(abPubKey0, &usSize0, KCMVP_ARIA128_KEY, 0);
    if (ret == KSE_SUCCESS)
      printf("Success kcmvpGetKey: %d\n", ret);
    else
      printf("Error kcmvpGetKey: %d\n", ret);

    printf("kcmvpGetKey Size: %d\n", usSize0);
    printf("kcmvpGetKey: ");
    for (int i = 0; i < usSize0; i++) {
      printf("%02X", abPubKey0[i]);
    }

    printf("\r\n");
    return 0;
}

int dim_set_key(uint8_t *abPubKey0) {
    int ret;

    // set key 0
    ret = _kcmvpEraseKey(KCMVP_ARIA128_KEY, 0);
    if (ret == KSE_SUCCESS)
      printf("Success kcmvpEraseKey: %d\n", ret);
    else
      printf("Error kcmvpEraseKey: %d\n", ret);

    ret = _kcmvpPutKey(KCMVP_ARIA128_KEY, 0, abPubKey0, 16);
    if (ret == KSE_SUCCESS)
      printf("Success kcmvpPutKey: %d\n", ret);
    else
      printf("Error kcmvpPutKey: %d\n", ret);

    return 0;
}

int dim_generate_key() {
    int16_t ret;

    ret = _kcmvpEraseKey(KCMVP_ARIA128_KEY, 0);
    if (ret == KSE_SUCCESS)
      printf("Success kcmvpEraseKey: %d\n", ret);
    else
      printf("Error kcmvpEraseKey: %d\n", ret);

    ret = _kcmvpGenerateKey(KCMVP_ARIA128_KEY, 0, NOT_USED);
    if (ret == KSE_SUCCESS )
      printf("Success kcmvpGenerateKey: %d\n", ret);
    else 
      printf("Error kcmvpGenerateKey: %d\n", ret);

    return ret;
}

int dim_generate_random() {
    int i;
    int16_t sRv;
    uint16_t usSize0, usSize1;
    uint8_t abData[256];

    // DRBG ====================================================================
    sRv = _kcmvpDrbg(abData, 256);
    if (sRv == KSE_SUCCESS)
        printf("Success kcmvpDrbg: %d\n", sRv);
    else
    {
        printf("Error kcmvpDrbg: Fail(-0x%04X)\r\n", -sRv);
        return -1;
    }
    printf("  * Random Number :\r\n    ");
    for (i = 0; i < 256; i++)
    {
        printf("%02X", abData[i]);
        if ((i < 255) && ((i + 1) % 32 == 0))
            printf("\r\n    ");
    }
    printf("\r\n");

    return 0;
}


int main(int argc, const char *argv[])
{
  if (argc !=2) {
    fprintf(stderr, "Usage: %s 10.243.45.201 \n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int mavlink_sock, commander_sock;
  mavlink_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  init_commander(commander_sock);

  DimClient *dim = new DimClient(argv[1], 4433);
  MAVLinkTlsClient *client = new MAVLinkTlsClient(mavlink_sock, dim);

  g_client = client;
  signal(SIGINT, signal_exit);

  // start_client_threads(client, dim);
  // wait_client_threads();

  // while 1
  // read command
  // if connect command: stop thread (run = 0) and start thread (run = 1)
  // if disconnect command: stop thread (run = 0)
  // if auth command with id, pass: send server and read key
  // if encrypt command with plaintext:
  // if decrypt command with cypertext:

  char commander_buffer[40];
  int received = 0;

  while (1) {
    sleep(1);
    if ((received = recvfrom(commander_sock, commander_buffer, 40, 0, NULL, NULL)) != -1) {
      if (received > 4) {
        printf("Received command: %s", commander_buffer);
        char * command = new char[4]();
        memcpy(command, &commander_buffer[0], 4);

        if (strncmp(command, "conn", 4) == 0) { // connect
          client->run = 0;
          wait_client_threads();
          client->run = 1;

          dim->open(argv[1], 4433);
          start_client_threads(client, dim);
        }
        if (strncmp(command, "disc", 4) == 0) { // disconnect
          client->run = 0;
          wait_client_threads();
        }
        if (strncmp(command, "auth", 4) == 0) { // auth
          client->run = 0;
          wait_client_threads();

          char *auth_key = new char[received - 5]();
          memcpy(auth_key, &commander_buffer[5], received - 5);
          printf("auth key: %s", auth_key);
        }
        if (strncmp(command, "decr", 4) == 0) { // decrypt
          client->run = 0;
          wait_client_threads();

	  /*
          char *plaintext = new char[received - 8]();
          memcpy(plaintext, &commander_buffer[8], received - 8);
          printf("plaintext: %s", plaintext);
	  */

           printf("dim_decrypt() \r\n");
          //dim_decrypt(1, 1, buffer, plaintext)

        }

        if (strncmp(command, "rand", 4) == 0) { // random
          client->run = 0;
          wait_client_threads();
    	  dim_generate_random();
	}

        if (strncmp(command, "encr", 4) == 0) { // encrypt
          client->run = 0;
          wait_client_threads();

          char *cypertext = new char[received - 8]();
          memcpy(cypertext, &commander_buffer[8], received - 8);
          //printf("cypertext: %s", cypertext);

    uint8_t abPubKey0[64];
    abPubKey0[0] = 0x03;
    abPubKey0[1] = 0x10;
    abPubKey0[2] = 0x81;
    abPubKey0[3] = 0x8A;
    abPubKey0[4] = 0x36;
    abPubKey0[5] = 0xE2;
    abPubKey0[6] = 0xCB;
    abPubKey0[7] = 0x32;
    abPubKey0[8] = 0x0A;
    abPubKey0[9] = 0xFD;
    abPubKey0[10] = 0x92;
    abPubKey0[11] = 0xEC;
    abPubKey0[12] = 0xE3;
    abPubKey0[13] = 0x52;
    abPubKey0[14] = 0x3D;
    abPubKey0[15] = 0x1A;

    dim_set_key((uint8_t*)abPubKey0);
    //dim_generate_key();
    //dim_get_key();

    int i;
    int16_t sRv;
    uint8_t abData[256], abTag[16];
    uint8_t abData1[256], abData2[256];
    uint8_t abIv[16], abAuth[128];

    // IV
    sRv = _kcmvpDrbg(abIv, 16);
    if (sRv != KSE_SUCCESS)
    {
        printf("_kcmvpDrbg(IV) : Fail(-0x%04X)\r\n", -sRv);
    }

    // Auth
    sRv = _kcmvpDrbg(abAuth, 128);
    if (sRv != KSE_SUCCESS)
    {
        printf("_kcmvpDrbg(Auth) : Fail(-0x%04X)\r\n", -sRv);
    }

    // Plaintext
    printf("  * Plain Data :\r\n    ");
    for (i = 0; i < 256; i++)
    {
        abData[i]= 0xaa;
        printf("%02X", abData[i]);
        if ((i < 255) && ((i + 1) % 32 == 0))
            printf("\r\n    ");
    }

    printf("\r\ndim_encrypt() \r\n");
    dim_encrypt(abData, 256, abIv, 16, abAuth, 128, abData1, 256, abTag, 16);

    printf("\r\n");

    uint8_t result [16+128+16+256];
    memcpy(result, &abIv[0], 16);
    memcpy(result+16, &abAuth[0], 128);
    memcpy(result+16+128, &abTag[0], 16);
    memcpy(result+16+128+16, &abData1[0], 256);

    printf("  * Result Data :\r\n    ");
    for (i = 0; i < 16+128+16+256; i++)
    {
        printf("%02X", result[i]);
        if ((i < 512) && ((i + 1) % 32 == 0))
            printf("\r\n    ");
    }
    printf("\r\n");
    printf("\r\n");
    printf("TEST");
    printf("\r\n");

    // TEST
    // Encrypt
    printf("  * Encrypted Data :\r\n    ");
    for (i = 0; i < 256; i++)
    {
        printf("%02X", abData1[i]);
        if ((i < 255) && ((i + 1) % 32 == 0))
            printf("\r\n    ");
    }
    printf("\r\n");

    // Tag
    printf("  * Tag :\r\n    ");
    for (i = 0; i < 16; i++)
        printf("%02X", abTag[i]);
    printf("\r\n");

    // decrypt
    dim_decrypt(1, 1, (const char*)result, abData2);


        }
      }
    }
  }

  dim->close();

  return EXIT_SUCCESS;
}

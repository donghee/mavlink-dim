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

// #define BUFFER_LENGTH 3072
#define BUFFER_LENGTH 8192

#include "readerwriterqueue.h"

using namespace dronemap;

moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_gcs;
moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_autopilot;

static volatile sig_atomic_t run = 1;
static DimSocket* dim;

static struct sockaddr_in gcAddr;
static struct sockaddr_in locAddr;
static int sock;
static bool first_run = false;

void exit_app(int signum)
{
  run = 0;
  std::cout << "Caught signal " << signum << std::endl;
}

uint64_t microsSinceEpoch()
{
  struct timeval tv;
  uint64_t micros = 0;

  gettimeofday(&tv, NULL);
  micros =  ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;

  return micros;
}

void debug_mavlink_msg_buffer(uint8_t* buffer, int buffer_size) {
    printf("MAVLink Buffer: 0x");
    for (int i = 0; i < buffer_size; ++i)
        printf("%X", buffer[i]);
    printf("\n");

    return;
}

// gcs -> autopilot
void gcs_read_message() {
  uint8_t recv_buffer[BUFFER_LENGTH];
  int16_t recv_size;
  socklen_t fromlen = sizeof(gcAddr);

  static mavlink_message_t message;
  mavlink_status_t status;

  bool received = false;
  while (!received) {
      // dim->connect(); // TODO: no new connection!
      recv_size = recvfrom(sock, (void *)recv_buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&gcAddr, &fromlen);
      if (recv_size > 0)
      {
          // printf("GCS YES Received\r\n");
          //dim->send(recv_size, recv_buffer);
          for (int i = 0; i < recv_size; ++i)
          {
              if (mavlink_parse_char(MAVLINK_COMM_0, recv_buffer[i], &message, &status))
              {
                  q_to_autopilot.enqueue(message);
                  received = true;
              }
          }
      } else {
        // printf("GCS Not Received\r\n");
	usleep(10);
      }
  }
}



void autopilot_write_message() {
    uint8_t send_buffer[BUFFER_LENGTH];
    int16_t send_size;

    mavlink_message_t message;

    // Fully-blocking
    q_to_autopilot.wait_dequeue(message);

    printf("Received message from gcs with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);

    send_size = mavlink_msg_to_send_buffer((uint8_t*)send_buffer, &message);
    // debug
    debug_mavlink_msg_buffer(send_buffer, send_size);

    dim->send(send_size, send_buffer);
    first_run = true;
}

// autopilot -> qgc
void autopilot_read_message() {
  int result;
  uint8_t recv_buffer[BUFFER_LENGTH];
  int16_t recv_size;

  static mavlink_message_t message;
  mavlink_status_t status;

  bool received = false;
  //while (!received && first_run) {
  while (!received) {
    result = dim->recv(&recv_size, recv_buffer);
    if ( result > 0 ) {
        //received = true;
      for (int i = 0; i < recv_size; ++i)
      {
        if (mavlink_parse_char(MAVLINK_COMM_0, recv_buffer[i], &message, &status))
        {
            q_to_gcs.enqueue(message);
            received = true;
        }
      }
    }
  }
}

void gcs_write_message() {
    uint8_t send_buffer[BUFFER_LENGTH];
    int16_t send_size;

    mavlink_message_t message;

    // Fully-blocking
    q_to_gcs.wait_dequeue(message);

    printf("Received message from fc with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);

    send_size = mavlink_msg_to_send_buffer((uint8_t*)send_buffer, &message);
    // debug
    debug_mavlink_msg_buffer(send_buffer, send_size);
 
    sendto(sock, send_buffer, send_size, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
}

// read and write
void* start_gcs_read_thread(void *args)
{
  while(run) {
      gcs_read_message();
      //usleep(1000); // 1000hz
      usleep(10); // 100000hz
  }
  return NULL;
}

void* start_autopilot_write_thread(void *args)
{
  while(run) {
      autopilot_write_message();
      usleep(100000); // 10hz
  }
  return NULL;
}

// read and write
void* start_autopilot_read_thread(void *args)
{
  dim->init_poll();
  while(run) {
      autopilot_read_message();
      //usleep(100); // 10000hz
      usleep(10); // 10000hz
  }
  return NULL;
}

void* start_gcs_write_thread(void *args)
{
  while(run) {
      gcs_write_message();
      usleep(1000); // 1000hz
  }
  return NULL;
}


int main(int argc, const char *argv[])
{
  char target_ip[100];
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  strcpy(target_ip, "127.0.0.1");

  memset(&locAddr, 0, sizeof(locAddr));
  locAddr.sin_family = AF_INET;
  locAddr.sin_addr.s_addr = INADDR_ANY;
  locAddr.sin_port = htons(14551);

  /* Bind the socket to port 14551 - necessary to receive packets from qgroundcontrol */
  if (-1 == bind(sock,(struct sockaddr *)&locAddr, sizeof(struct sockaddr)))
  {
    std::cout << "bind fail " << std::endl;
    close(sock);
    return -1;
  }

  if (fcntl(sock, F_SETFL, O_NONBLOCK| O_ASYNC) < 0)

  // QGC
  memset(&gcAddr, 0, sizeof(gcAddr));
  gcAddr.sin_family = AF_INET;
  gcAddr.sin_addr.s_addr = inet_addr(target_ip);
  gcAddr.sin_port = htons(14550);

  signal(SIGINT, exit_app);

  // dim
  dim = new DimSocket(4433);

  // pthread
  int result;
  pthread_t autopilot_read_tid, gcs_write_tid, gcs_read_tid, autopilot_write_tid;

  result = pthread_create( &autopilot_read_tid, NULL, &start_autopilot_read_thread, (char*)"Autopilot Reading" );
  if ( result ) throw result;

  result = pthread_create( &gcs_write_tid, NULL, &start_gcs_write_thread, (char*)"GCS Writing" );
  if ( result ) throw result;

  result = pthread_create( &gcs_read_tid, NULL, &start_gcs_read_thread, (char*)"GCS Reading" );
  if ( result ) throw result;

  result = pthread_create( &autopilot_write_tid, NULL, &start_autopilot_write_thread, (char*)"Autopilot Writing" );
  if ( result ) throw result;

  // wait for exit
  pthread_join(autopilot_read_tid, NULL);
  pthread_join(gcs_write_tid, NULL);
  pthread_join(gcs_read_tid, NULL);
  pthread_join(autopilot_write_tid, NULL);

  dim->close();

  return 0;
}

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

using namespace dronemap;

static volatile sig_atomic_t run = 1;
static DimSocket* dim;

static struct sockaddr_in gcAddr;
static struct sockaddr_in locAddr;
static int sock;

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

void gc_worker() {
  uint8_t gc_buffer[BUFFER_LENGTH];
  int16_t recv_size;
  int bytes_sent;

  static mavlink_message_t msg;
  mavlink_status_t status;

  while(run) {
    // fc -> qgc
    int result = -1;
    //memset(gc_buffer, 0, sizeof(gc_buffer));
    result = dim->recv(&recv_size, gc_buffer);
    if ( result > 0 ) {
      // bytes_sent = sendto(sock, gc_buffer, recv_size, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
      // printf("\r\nfrom fc %d \r\n", recv_size);
      for (int i = 0; i < recv_size; ++i)
      {
        if (mavlink_parse_char(MAVLINK_COMM_0, gc_buffer[i], &msg, &status))
        {
          // Packet received
          // printf("packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
          bytes_sent = sendto(sock, gc_buffer, recv_size, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
        }
      }
      // usleep(10000);
    }
  }
}

void fc_worker() {
  uint8_t fc_buffer[BUFFER_LENGTH];
  int16_t recv_size;;
  socklen_t fromlen = sizeof(gcAddr);

  static mavlink_message_t msg;
  mavlink_status_t status;

  while (run) {
    try {
      // qgc -> fc
      // dim->connect(); // TODO: no new connection!
      //      memset(fc_buffer, 0, sizeof(fc_buffer));
      recv_size = recvfrom(sock, (void *)fc_buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&gcAddr, &fromlen);
     if (recv_size > 0)
      {
        //dim->send(recv_size, fc_buffer);
        // Something received - print out all bytes and parse packet

        printf("\r\nfrom qgc %d\r\n",recv_size);
        for (int i = 0; i < recv_size; ++i)
        {
          // temp = fc_buffer[i];
          // printf("%02x ", (unsigned char)temp);
          if (mavlink_parse_char(MAVLINK_COMM_0, fc_buffer[i], &msg, &status))
          {
            // Packet received
            printf("packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
            //            dim->send(recv_size, fc_buffer);
          }
        }
      }
    } catch (...) {
      std::cout << " catch runtime error (...) " << std::endl;
      continue;
      // dim->close();
    }

  }
}



int main(int argc, const char *argv[])
{
  // mavlink
  char target_ip[100];
  float position[6] = {};
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int bytes_sent;
  mavlink_message_t msg;
  uint16_t len;
  int i = 0;
  //int success = 0;
  unsigned int temp = 0;
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
  static auto _dim = DimSocket(4433);
  dim = &_dim;

  std::thread t0(gc_worker);
  // std::thread t1(fc_worker);

  t0.join();
  //  t1.join();

  //dim->close();


  return 0;
}

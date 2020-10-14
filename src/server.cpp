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

using namespace dronemap;

//#define BUFFER_LENGTH 2048
#define BUFFER_LENGTH 8192

static volatile sig_atomic_t run = 1;
static SerialPort* port;
static DimSocket* dim;
static volatile bool autopilot_reading_state = false;

void exit_app(int signum)
{
  run = 0;
  std::cout << "Caught signal " << signum << std::endl;
}

// qgc -> fc
void gc_worker() {
  uint8_t gc_buffer[BUFFER_LENGTH];
  int16_t recv_size;
  printf("\r\nbefore dim listen\r\n");
  dim->listen();
  printf("\r\nafter dim listen\r\n");
  while(run) {
    int result = -1;
    //    printf("\r\nbefore dim recv\r\n");
    // result = dim->recv(&recv_size, gc_buffer);
    //    printf("\r\nafter dim recv\r\n");

    if (result > 0) {
      if(port){
        port->send(recv_size, gc_buffer);
        printf("\r\nfrom qgc, %d\r\n", recv_size);
        //      for (uint16_t i = 0; i < recv_size ; i++) {
        //        printf("%02X ", gc_buffer[i]);
        //      }
        //printf("\r\n");
      }
    }
    usleep(10000);
  }
}

// fc -> qgc
void fc_worker() {
  uint8_t fc_buffer[BUFFER_LENGTH];
  int16_t recv_size;
  int result;
  bool received = false;
  mavlink_message_t message;

  mavlink_sys_status_t sys_status;
  mavlink_auth_key_t auth_key;
  mavlink_heartbeat_t heartbeat;

  // Heart beat
  // mavlink_msg_heartbeat_encode(255, 190, &message, &heartbeat);
  // mavlink_msg_to_send_buffer((uint8_t*)fc_buffer, &message);
  // printf("\r\nPort send");
  // port->send(sizeof(fc_buffer), fc_buffer);
  // printf("\r\nPort send");

  while (!received) {
    // try {
      result = port->read_message(message);
      if (result > 0) {

          received = true;
          printf("\r\nReceived message from serial with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);

          // modify sys status
          if (message.msgid == MAVLINK_MSG_ID_SYS_STATUS) {
              mavlink_msg_sys_status_decode(&message, &sys_status);
              sys_status.errors_count4 = 1;
              mavlink_msg_sys_status_encode(1, 1, &message, &sys_status);
              // memset(auth_key.key, '\0', 32);
              // mavlink_msg_auth_key_encode(1, 1, &message, &auth_key);
          }

        switch (message.msgid)
        {
          case MAVLINK_MSG_ID_HEARTBEAT:
               printf("MAVLINK_MSG_ID_HEARTBEAT\n");
          case MAVLINK_MSG_ID_SYS_STATUS:
          case MAVLINK_MSG_ID_AUTH_KEY:
              //case MAVLINK_MSG_ID_HIGHRES_IMU:
              //case MAVLINK_MSG_ID_ATTITUDE:
          case MAVLINK_MSG_ID_ATTITUDE_QUATERNION:
              //case MAVLINK_MSG_ID_ALTITUDE:
          {
              recv_size = mavlink_msg_to_send_buffer((uint8_t*)fc_buffer, &message);
              if (dim->is_connected()) {
                dim->send(recv_size, fc_buffer);
              }

          }
        }
      }
    // } catch (...) {
    //   std::cout << " catch runtime error (...) " << std::endl;
    //   continue;
    // }
  }
}


void* start_autopilot_read_thread(void *args)
{
  while(run) {
    fc_worker();
    //    usleep(100000); // 10hz
    //    usleep(50000); // 20hz
    usleep(10000); // 100hz
  }
  return NULL;
}

void* start_autopilot_write_thread(void *args)
{
  gc_worker();
  return NULL;
}

int main(int argc, const char *argv[])
{
  int result;
  pthread_t read_tid, write_tid;

  port = new SerialPort("/dev/ttyACM0", 57600);
  dim = new DimSocket(4433, true);

  result = pthread_create( &read_tid, NULL, &start_autopilot_read_thread, (char*)"Autopilot Reading" );
  if ( result ) throw result;

  result = pthread_create( &write_tid, NULL, &start_autopilot_write_thread, (char*)"Autopilot Writing" );
  if ( result ) throw result;

  // wait for exit
  pthread_join(read_tid, NULL);

  // wait for exit
  pthread_join(write_tid, NULL);

  //  std::thread t0(gc_worker);
  //  std::thread t1(fc_worker);

  //  t0.join();
  //  t1.join();

  //dim->close();
  //port->close();

  return 0;
}

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

#include "readerwriterqueue.h"

using namespace dronemap;

moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_gcs;
moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_autopilot;

//#define BUFFER_LENGTH 2048
#define BUFFER_LENGTH 8192

static volatile sig_atomic_t run = 1;
static Serial_Port* port;
static DimSocket* dim;
static volatile bool autopilot_reading_state = false;

void exit_app(int signum)
{
  run = 0;
  std::cout << "Caught signal " << signum << std::endl;
}


// autopilot -> gcs
void autopilot_read_message() {
    int result;
    bool received = false;
    mavlink_message_t message;

    while (!received) {
        result = port->read_message(message);
        if (result > 0) {
            q_to_gcs.enqueue(message);
            received = true;
        }
    }
}

void gcs_write_message() {
    uint8_t send_buffer[BUFFER_LENGTH];
    int16_t recv_size;

    mavlink_message_t message;

    mavlink_sys_status_t sys_status;
    // mavlink_auth_key_t auth_key;

    // Fully-blocking
    q_to_gcs.wait_dequeue(message);

    // printf("Received message from serial with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);

    // Modify sys status
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
        case MAVLINK_MSG_ID_SYS_STATUS:
        // case MAVLINK_MSG_ID_AUTH_KEY:
            //case MAVLINK_MSG_ID_HIGHRES_IMU:
            //case MAVLINK_MSG_ID_ATTITUDE:
        // case MAVLINK_MSG_ID_ATTITUDE_QUATERNION:
            //case MAVLINK_MSG_ID_ALTITUDE:
            {
                recv_size = mavlink_msg_to_send_buffer((uint8_t*)send_buffer, &message);
                if (dim && dim->is_connected()) {
                    dim->send(recv_size, send_buffer);
                }
            }
    }
}

// gcs -> autopilot
void gcs_read_message() {
    int result = -1;
    bool received = false;
    static mavlink_message_t message;
    mavlink_status_t status;

    uint8_t gc_buffer[BUFFER_LENGTH];
    int16_t recv_size;

    while(!received) {

        result = dim->recv(&recv_size, gc_buffer);
        if (result >= 0) {
            for (int i = 0; i < recv_size; ++i)
            {
                if (mavlink_parse_char(MAVLINK_COMM_0, gc_buffer[i], &message, &status))
                {
                    q_to_autopilot.enqueue(message);
                    received = true;
                }
            }
        }
     }
}

void autopilot_write_message() {
    mavlink_message_t message;

    // Fully-blocking
    q_to_autopilot.wait_dequeue(message);

    printf("Received message from gcs with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);

    if(port){
        port->write_message(message);
    }
}

void* start_autopilot_read_thread(void *args)
{
    while(run) {
        autopilot_read_message();
    //    usleep(100000); // 10hz
    //    usleep(50000); // 20hz
       usleep(10000); // 100hz
        // usleep(1000); // 1000hz
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

void* start_gcs_read_thread(void *args)
{
  printf("\r\nbefore dim listen\r\n");
  try {
      dim->listen();
  } catch (...) {
      std::cout << " catch runtime error (listen)" << std::endl;
      dim->close();
      return NULL;
  }
  printf("\r\nafter dim listen\r\n");
  dim->init_poll();
  while(run) {
      gcs_read_message();
      // usleep(1000000); // 1hz
      usleep(10000); // 100hz very important
  }
  return NULL;
}

void* start_autopilot_write_thread(void *args)
{
    while(run) {
        autopilot_write_message();
    //    usleep(100000); // 10hz
    //    usleep(50000); // 20hz
    //    usleep(10000); // 100hz
        usleep(1000); // 1000hz
  }
  return NULL;
}


int main(int argc, const char *argv[])
{
  int result;
  pthread_t autopilot_read_tid, gcs_read_tid, gcs_write_tid, autopilot_write_tid;

  port = new Serial_Port("/dev/ttyACM0", 57600);
  port->start();

  result = pthread_create( &autopilot_read_tid, NULL, &start_autopilot_read_thread, (char*)"Autopilot Reading" );
  if ( result ) throw result;

  dim = new DimSocket(4433, true);

  result = pthread_create( &gcs_write_tid, NULL, &start_gcs_write_thread, (char*)"GCS Writing" );
  if ( result ) throw result;

  result = pthread_create( &gcs_read_tid, NULL, &start_gcs_read_thread, (char*)"GCS Reading" );
  if ( result ) throw result;

  result = pthread_create( &autopilot_write_tid, NULL, &start_autopilot_write_thread, (char*)"Autopilot Writing" );
  if ( result ) throw result;

  // wait for exit
  pthread_join(autopilot_read_tid, NULL);

  // wait for exit
  pthread_join(gcs_write_tid, NULL);

  // wait for exit
  pthread_join(gcs_read_tid, NULL);

  // wait for exit
  pthread_join(autopilot_write_tid, NULL);

  //  std::thread t0(gc_worker);
  //  std::thread t1(fc_worker);

  //  t0.join();
  //  t1.join();

  //dim->close();
  //port->close();

  return 0;
}

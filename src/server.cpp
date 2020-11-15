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

#define BUFFER_LENGTH 8192

#include "readerwriterqueue.h"

using namespace dronemap;

moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_gcs;
moodycamel::BlockingReaderWriterQueue<mavlink_message_t> q_to_autopilot;

static volatile sig_atomic_t run = 1;
static Serial_Port* port;
static DimSocket* dim;

static volatile int autopilot_writing_status = 0;

static volatile int dim_writing_status = 0;

void exit_app(int signum)
{
  run = 0;
  std::cout << "Caught signal " << signum << std::endl;
  port->stop();
  dim->close();
  dim->power_off();
  exit(1);
}

void debug_mavlink_msg_buffer(uint8_t* buffer, int buffer_size) {
    printf("MAVLink Buffer: 0x");
    for (int i = 0; i < buffer_size; ++i)
        printf("%X", buffer[i]);
    printf("\n");

    return;
}

// autopilot -> gcs
void autopilot_read_message() {
    int result;
    bool received = false;
    bool received_heartbeat = false;
    bool received_sys_status = false;
    bool received_attitude = false;
    mavlink_message_t message;

    while (!received) {
        result = port->read_message(message);
        if (result > 0) {
            if (
            // message.msgid == MAVLINK_MSG_ID_ATTITUDE ||
            // message.msgid == MAVLINK_MSG_ID_ATTITUDE_QUATERNION ||
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
            // message.msgid == MAVLINK_MSG_ID_ODOMETRY ||
            message.msgid == MAVLINK_MSG_ID_ALTITUDE )  {
                continue;
            }
            q_to_gcs.enqueue(message);

            if(message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
               received_heartbeat = true;
            }
            received = received_heartbeat;
        }
        usleep(10); // 10kHz
        if ( autopilot_writing_status > false )
            usleep(100); // 10kHz
    }
}

int gcs_write_message() {
    int result = 0;
    uint8_t send_buffer[BUFFER_LENGTH];
    int16_t recv_size;

    mavlink_message_t message;
    mavlink_sys_status_t sys_status;
    // mavlink_auth_key_t auth_key;

    // Fully-blocking
    q_to_gcs.wait_dequeue(message);
    // if (q_to_gcs.wait_dequeue_timed(message, std::chrono::milliseconds(5)))
    {
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
        recv_size = mavlink_msg_to_send_buffer((uint8_t*)send_buffer, &message);
        // debug
        //debug_mavlink_msg_buffer(send_buffer, recv_size);
        if (dim->is_connected()) {
            dim_writing_status = true;
            result = dim->send(recv_size, send_buffer);
            dim_writing_status = false;
            if ( result < 0) {
                sleep(1);
                return result;
            }
        }
    }
    return 0;
}

// gcs -> autopilot
int gcs_read_message() {
    int result = 0;
    bool received = false;
    mavlink_message_t message;
    mavlink_status_t status;

    uint8_t recv_buffer[BUFFER_LENGTH];
    int16_t recv_size;

    while(!received && dim->is_connected()) {
        result = dim->recv(&recv_size, recv_buffer);
        if (result > 0) {
            for (int i = 0; i < recv_size; ++i)
            {
                if (mavlink_parse_char(MAVLINK_COMM_0, recv_buffer[i], &message, &status))
                {
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
        if ( dim_writing_status > false )
            usleep(100); // 10kHz
     }
     return 1;
}

void autopilot_write_message() {
    uint8_t recv_buffer[BUFFER_LENGTH];
    int16_t recv_size;

    mavlink_message_t message;

    // Fully-blocking
    q_to_autopilot.wait_dequeue(message);
    // Blocking with timeout
    // if (q_to_autopilot.wait_dequeue_timed(message, std::chrono::milliseconds(5)))
    {
        printf("Received message from gcs with ID #%d (sys:%d|comp:%d):\r\n", message.msgid, message.sysid, message.compid);
        // debug
        //recv_size = mavlink_msg_to_send_buffer((uint8_t*)recv_buffer, &message);
        //debug_mavlink_msg_buffer(recv_buffer, recv_size);

        autopilot_writing_status = true;
        if(port){
            port->write_message(message);
        }
        autopilot_writing_status = false;
    }
}

void* start_autopilot_read_thread(void *args)
{
  while(run) {
        autopilot_read_message();
        usleep(10); // 10khz
  }
  printf("\r\nExit autopilot read thread\r\n");
  return NULL;
}

void* start_gcs_write_thread(void *args)
{
    int result;
    while(run) {
        result = gcs_write_message();
        if (result < 0) {
            printf("\r\nSEND ERROR: %X\r\n", result);
        }
        usleep(10); // 10khz
    }
    printf("\r\nExit gcs write thread\r\n");
    return NULL;
}

void* start_gcs_read_thread(void *args)
{
  int result;
  int no_data_count = 0; // no data counts

  while(run) {
    if (dim->accept() == -1) {
        // dim->close();
        printf("\r\nACCEPT failed\r\n");
        continue;
    }
    dim->init_poll();
    printf("\r\nStart gcs read message\r\n");
    while(1) {
        result = gcs_read_message();
        if (result < 0) { // ERROR
            if (result == -0x7880) {
                printf("\r\nGot KSETLS_ERROR_TLS_PEER_CLOSE_NOTIFY\r\n");
                continue;
            } else if (result == -0x7780) {
                printf("\r\nGot KSETLS_ERROR_TLS_FATAL_ALERT_MESSAGE\r\n");
                sleep(1);
                result = dim->tls_close_notify();
                dim->close();
                break;
            } else {
                printf("\r\nDIM READ ERROR: %X\r\n", result);
            }
            break;
        }
        if (result == 0) { // NO DATA
            no_data_count += 1;
            if (no_data_count > 10000) {
                printf("\r\NO DATA: %d\r\n", no_data_count);
                sleep(1);
                result = dim->tls_close_notify();
                dim->close();
                no_data_count = 0;
                break;
            }
            continue;
        } else { // ON DATA
            printf("\r\NO DATA: %d\r\n", no_data_count);
            no_data_count = 0;
        }
        usleep(10);
    }
  }
  printf("\r\nExit gcs read thread\r\n");
  return NULL;
}

void* start_autopilot_write_thread(void *args)
{
  while(run) {
        autopilot_write_message();
        usleep(1000); // 1000hz
        //usleep(100000); // 10hz
      //usleep(10); // test
  }
  printf("\r\nExit autopilot write thread\r\n");
  return NULL;
}


int main(int argc, const char *argv[])
{
  int result;
  pthread_t autopilot_read_tid, gcs_read_tid, gcs_write_tid, autopilot_write_tid;

  // signal(SIGINT, exit_app);

  port = new Serial_Port("/dev/ttyACM0", 57600);
  port->start();

  try {
      dim = new DimSocket(4433, true);
  } catch (...) {
      std::cout << " catch runtime error (dim init)" << std::endl;
      return -1;
  }

  result = pthread_create( &autopilot_read_tid, NULL, &start_autopilot_read_thread, (char*)"Autopilot Reading" );
  if ( result ) throw result;

  result = pthread_create( &gcs_write_tid, NULL, &start_gcs_write_thread, (char*)"GCS Writing" );
  if ( result ) throw result;

  result = pthread_create( &gcs_read_tid, NULL, &start_gcs_read_thread, (char*)"GCS Reading" );
  if ( result ) throw result;

  result = pthread_create( &autopilot_write_tid, NULL, &start_autopilot_write_thread, (char*)"Autopilot Writing" );
  if ( result ) throw result;

  // wait for exit
  pthread_join(gcs_write_tid, NULL);

  // wait for exit
  pthread_join(gcs_read_tid, NULL);

  // wait for exit
  pthread_join(autopilot_write_tid, NULL);

  // wait for exit
  pthread_join(autopilot_read_tid, NULL);

  dim->close();
  port->stop();

  return 0;
}

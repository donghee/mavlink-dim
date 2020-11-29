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

  while (!received) {
    result = port->read_message(message);

    if (result > 0) {
      if (
              message.msgid == MAVLINK_MSG_ID_ATTITUDE ||
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
              message.msgid == MAVLINK_MSG_ID_BATTERY_STATUS ||
              message.msgid == MAVLINK_MSG_ID_ALTITUDE)  {
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
  while (run) {
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
    if (dim->accept() == -1) {
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
          usleep(500000); // 0.5s
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
        printf("Timeout: %d\r\n", no_data_count);
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

int
start_server_threads(Serial_Port *port, DimSocket *dim)
{
  int result;
  pthread_t autopilot_read_tid, gcs_read_tid, gcs_write_tid, autopilot_write_tid;

  MAVLinkTlsServer *server;
  server = new MAVLinkTlsServer(port, dim);
  g_server = server;

  signal(SIGINT, signal_exit);

  //result = pthread_create(&autopilot_read_tid, NULL, &start_autopilot_read_thread, (char *)"Autopilot Reading");
  result = pthread_create(&autopilot_read_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsServer::start_autopilot_read_thread, server);

  if (result) { throw result; }

  result = pthread_create(&gcs_write_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsServer::start_gcs_write_thread, server);

  if (result) { throw result; }

  result = pthread_create(&gcs_read_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsServer::start_gcs_read_thread, server);

  if (result) { throw result; }

  result = pthread_create(&autopilot_write_tid, NULL, (THREADFUNCPTR) &MAVLinkTlsServer::start_autopilot_write_thread, server);

  if (result) { throw result; }

  pthread_join(gcs_write_tid, NULL);
  pthread_join(gcs_read_tid, NULL);
  pthread_join(autopilot_write_tid, NULL);
  pthread_join(autopilot_read_tid, NULL);

  return NULL;
}

int main(int argc, const char *argv[])
{
  Serial_Port *port;
  DimSocket *dim;

  //port = new Serial_Port("/dev/ttyACM0", 57600);
  port = new Serial_Port("/dev/ttyS0", 921600);
  port->start();

  dim = new DimSocket(4433, true);

  start_server_threads(port, dim);

  dim->close();
  port->stop();

  return 0;
}

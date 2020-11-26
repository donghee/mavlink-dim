/**
 * @file server.h
 *
 * @brief DIM TLS server
 *
 * TCP TLS server code for communication MAVLink endpoints in MC
 *
 * @author Donghee Park,   <dongheepark@gmail.com>
 *
 */

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


void exit_app(int signum);

/* print mavlink bytes for debugging
 * @param buffer bytes to print
 * @param size of bytes to print
 * @return
 */
void debug_mavlink_msg_buffer(uint8_t *buffer, int buffer_size);

// autopilot -> gcs
void autopilot_read_message();

int gcs_write_message();

// gcs -> autopilot
int gcs_read_message();

void autopilot_write_message();

void *start_autopilot_read_thread(void *args);

void *start_gcs_write_thread(void *args);

void *start_gcs_read_thread(void *args);

void *start_autopilot_write_thread(void *args);

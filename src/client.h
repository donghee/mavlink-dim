/**
 * @file client.h
 *
 * @brief DIM TLS Client
 *
 * TCP TLS client code for communication MAVLink endpoints in GCS
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

#include <mavlink.h>
#include <sys/time.h>

#define BUFFER_LENGTH 8192

#include "readerwriterqueue.h"

using namespace dronemap;

void exit_app(int signum);

uint64_t microsSinceEpoch();

/* print mavlink bytes for debugging
 * @param buffer bytes to print
 * @param size of bytes to print
 * @return
 */
void debug_mavlink_msg_buffer(uint8_t *buffer, int buffer_size);

// gcs -> autopilot
void gcs_read_message();

int autopilot_write_message();

// autopilot -> qgc
int autopilot_read_message();

void gcs_write_message();

// read and write
void *start_gcs_read_thread(void *args);

void *start_autopilot_write_thread(void *args);

// read and write
void *start_autopilot_read_thread(void *args);

void *start_gcs_write_thread(void *args);

void *start_autopilot_read_write_thread(void *args);

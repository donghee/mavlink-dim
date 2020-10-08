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

#define BUFFER_LENGTH 2048

static volatile sig_atomic_t run = 1;
static SerialPort* port;
static DimSocket* dim;

void exit_app(int signum)
{
    run = 0;
    std::cout << "Caught signal " << signum << std::endl;
}

int main(int argc, const char *argv[])
{
    uint8_t fc_buffer[BUFFER_LENGTH];
    int16_t recv_size;
    static auto _dim = DimSocket(4433, true);
    dim = &_dim;
    signal(SIGINT, exit_app);

    // pixhawk
    static SerialPort* _port = new SerialPort("/dev/ttyACM0", 57600);
    port = _port;

    std::thread([]() {
                    uint8_t gc_buffer[BUFFER_LENGTH];
                    int16_t recv_size;

                    while(run) {
                        int result=-1;
                        memset(gc_buffer, 0, sizeof(gc_buffer));
                        result = dim->recv(&recv_size, gc_buffer);
                        if (result > 0) {
                            port->send(recv_size, gc_buffer);
                            printf("\r\nfrom qgc\r\n");
                            for (uint16_t i = 0; i < recv_size ; i++) {
                                printf("%02X ", gc_buffer[i]);
                            }
                            printf("\r\n");
                        }

                    }
                }).detach();

    while (run) {
        try {
            int result=-1;
            memset(fc_buffer, 0, sizeof(fc_buffer));
            result = port->recv(&recv_size, fc_buffer);
            if (result > 0) {
                dim->send(recv_size, fc_buffer);
                printf("\r\nfrom fc\r\n");
                for (uint16_t i = 0; i < recv_size ; i++) {
                    printf("%02X ", fc_buffer[i]);
                }
                printf("\n");
            }
        } catch (...) {
            std::cout << " catch runtime error (...) " << std::endl;
            continue;
            // dim->close();
        }
    }

    dim->close();
    port->close();

    return 0;
}

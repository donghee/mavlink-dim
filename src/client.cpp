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
#define BUFFER_LENGTH 2048

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

int main(int argc, const char *argv[])
{

    // mavlink
    char target_ip[100];
    float position[6] = {};
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    uint8_t buf[2041];
    ssize_t recsize;
    socklen_t fromlen = sizeof(gcAddr);
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

    if (fcntl(sock, F_SETFL, O_NONBLOCK | O_ASYNC) < 0)

    // QGC
    memset(&gcAddr, 0, sizeof(gcAddr));
    gcAddr.sin_family = AF_INET;
    gcAddr.sin_addr.s_addr = inet_addr(target_ip);
    gcAddr.sin_port = htons(14550);

    // dim
    uint8_t fc_buffer[BUFFER_LENGTH];
    int16_t recv_size;
    static auto _dim = DimSocket(4433);
    dim = &_dim;
    signal(SIGINT, exit_app);

    std::thread([]() {
                    uint8_t gc_buffer[BUFFER_LENGTH];
                    int16_t recv_size;
                    int bytes_sent;

                    while(run) {
                        // fc -> qgc
                        int result = -1;
                        memset(gc_buffer, 0, sizeof(gc_buffer));
                        result = dim->recv(&recv_size, gc_buffer);

                        if ( result > 0 ) {
                            // bytes_sent = sendto(sock, gc_buffer, recv_size, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
                            mavlink_message_t msg;
                            mavlink_status_t status;

                            printf("\r\nfrom fc");
                            for (int i = 0; i < recv_size; ++i)
                            {
                                // temp = gc_buffer[i];
                                // printf("%02x ", (unsigned char)temp);
                                if (mavlink_parse_char(MAVLINK_COMM_0, gc_buffer[i], &msg, &status))
                                {
                                    // Packet received
                                    printf("\n packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
                                    bytes_sent = sendto(sock, gc_buffer, recv_size, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
                                }
                            }
                        }
                    }
                }).detach();

    while (run) {
        try {
            // qgc -> fc
            // dim->connect(); // TODO: no new connection!
            memset(fc_buffer, 0, sizeof(fc_buffer));
            recsize = recvfrom(sock, (void *)fc_buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&gcAddr, &fromlen);
            if (recsize > 0)
            {
                // dim->send(recsize, fc_buffer);
                // Something received - print out all bytes and parse packet
                mavlink_message_t msg;
                mavlink_status_t status;

                printf("\r\nfrom qgc");
                for (int i = 0; i < recsize; ++i)
                {
                    // temp = fc_buffer[i];
                    // printf("%02x ", (unsigned char)temp);
                    if (mavlink_parse_char(MAVLINK_COMM_0, fc_buffer[i], &msg, &status))
                    {
                        // Packet received
                        printf("\n packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
                        dim->send(recsize, fc_buffer);
                    }
                }
            }
        } catch (...) {
            std::cout << " catch runtime error (...) " << std::endl;
            continue;
            // dim->close();
        }

    }

    dim->close();


    return 0;
}

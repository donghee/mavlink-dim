#include <iostream>
#include <thread>

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <dim.h>
#include <signal.h>

using namespace dronemap;

volatile sig_atomic_t run = 1;
void exit_app(int signum) {
    run = 0;
    std::cout << "Caught signal " << signum << std::endl;
}

int main(int argc, const char *argv[])
{
    uint8_t buffer[3072];
    int16_t recv_size;
    auto dim = DimSocket(80);
    signal(SIGINT, exit_app);

	try {
		/*
		std::thread([]() {
		while (true) {
		printf("hello ");
		}
		}).detach();
        */

		while (run) {
            dim.connect(); // TODO: no new connection!
            memset(buffer, 0, sizeof(buffer));
            memcpy(buffer, "Echo this10", strlen("Echo this10"));
            dim.send(strlen("Echo this10"), buffer);

            memset(buffer, 0, sizeof(buffer));
            dim.recv(&recv_size, buffer);
		}

        dim.close();

	} catch (...) {
        std::cout << " catch runtime error (...) " << std::endl;
        dim.close();
	}

	return 0;
}

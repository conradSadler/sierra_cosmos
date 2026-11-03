#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
// Used for the SCNu32 macro which sets decimal scanf format for uint32_t
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>

#include "packet_def.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define RPI4B_ID 1

int init_con(int *server_fd, int *client_fd, uint16_t port);

int send_telemetry(int client_fd);
int receive_commands(int client_fd);
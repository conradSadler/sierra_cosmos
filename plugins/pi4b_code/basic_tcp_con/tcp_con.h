#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>

// #include "packet_def.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define RPI4B_ID 1

uint8_t init_con(uint32_t *server_fd, uint32_t *client_fd, 
    struct sockaddr_in *server_addr, struct sockaddr_in *client_addr, uint16_t port);

uint8_t send_telemetry(uint32_t client_fd);
uint8_t receive_commands(uint32_t client_fd);
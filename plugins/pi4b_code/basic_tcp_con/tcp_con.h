#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>

#include "message.h"
// #include "packet_def.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define RPI4B_ID 1

int32_t init_con(int32_t *server_fd, int32_t *client_fd, 
    struct sockaddr_in *server_addr, struct sockaddr_in *client_addr, uint16_t port);

int32_t payload_receive_message(int32_t client_fd);
int32_t host_receive_message(int32_t client_fd);
int32_t host_send_message(int32_t client_fd, uint16_t message_id);

int32_t send_telemetry(int32_t client_fd);
int32_t receive_commands(int32_t client_fd);

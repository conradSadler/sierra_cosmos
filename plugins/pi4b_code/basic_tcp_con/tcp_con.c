#include "tcp_con.h"

// static struct sockaddr_in server_addr;
// static struct sockaddr_in client_addr;

/*
    Returns 0 if successful
    Returns -1 if failure came up
*/
uint8_t init_con(
    uint32_t *server_fd, 
    uint32_t *client_fd,
    struct sockaddr_in *server_addr,
    struct sockaddr_in *client_addr,
    uint16_t port){

    socklen_t addr_len = sizeof(*client_addr);
    int opt = 1;
    
    // Create a socket
    if((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket creation failed");
        return -1;
    }
    if(setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt failed");
        close(*server_fd);
        return -1;
    }
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = INADDR_ANY;
    server_addr->sin_port = htons(port);

    // Bounding to a local address with bind
    if (bind(*server_fd, (struct sockaddr*)server_addr, sizeof(*server_addr)) < 0){
        perror("Bind failed");
        close(*server_fd);
        return -1;
    }

    // Listen with queue limit
    // https://man7.org/linux/man-pages/man2/listen.2.html
    if (listen(*server_fd, 3) < 0){
        perror("Listen failed");
        close(*server_fd);
        return -1;
    }
    printf("Server listening on %d...\n", port);

    if((*client_fd = accept(*server_fd, (struct sockaddr*)client_addr, &addr_len)) < 0){
        perror("Accept failed");
        close(*server_fd);
        return -1;
    }
    uint32_t flags = fcntl(*client_fd, F_GETFL, 0);
    fcntl(*client_fd, F_SETFL, flags | O_NONBLOCK);

    printf("Connection accepted from %s:%d\n", 
        inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    return 0;
}

uint8_t send_telemetry(uint32_t client_fd){
    uint32_t cpu_temp;

    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp == NULL){
        perror("Couldn't get file!");
        return -1;
    }
    //int items_matched = fscanf(fp, "%" SCNu32, &cpu_temp);
    int items_matched = fscanf(fp, "%d", &cpu_temp);
    fclose(fp);
    if(items_matched != 1){
        perror("Reading File Error");
        return -1;
    }
    printf("cpu_temp: %x\n", cpu_temp);

    // payload_t packet;
    // packet.first_two = htons(FIRST_TWO_OCTET(6, 1, 0, 1));
    // packet.packet_sequence_control = htons(SECOND_TWO_OCTET(1, seq_count++));
    // packet.packet_data_length = htons(sizeof(int32_t)-1);
    // packet.cpu_temp = htonl(cpu_temp);
    // //printf("size: %zu, %b\n", sizeof(packet), packet);
    // printf("size: %zu, hdr: %04x %04x %04x %08x\n", sizeof(packet),
    // //printf("size: %zu, hdr: %16b %16b %16b %04x\n", sizeof(packet),
    //    ntohs(packet.first_two), ntohs(packet.packet_sequence_control),
    //    ntohs(packet.packet_data_length), ntohl(packet.cpu_temp));
    // ssize_t bytes_sent = send(client_fd, &packet, sizeof(packet), MSG_NOSIGNAL);
    // if(bytes_sent != (ssize_t)sizeof(packet)){
    //     if(bytes_sent < 0){
    //         perror("Error sending bytes!\n");
    //     }
    //     else{
    //         fprintf(stderr, "Error sending bytes: short write\n");
    //     }
    //     return -1;
    // }
    // return 0;
}

uint8_t receive_commands(uint32_t client_fd){
    uint8_t buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
    if(bytes_read < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK ){
            return 0;
        }
        perror("Read failed");
        return -1;
    }
    printf("Recieved %zd bytes: %x\n", bytes_read, buffer);
    return 0;
}

// int main(){
//     uint32_t server_fd, client_fd;                       //File Descriptor

//     if(init_con(&server_fd, &client_fd, PORT) == -1){
//         printf("init_con error");
//         exit(EXIT_FAILURE);
//     }

//     while(1){
//         if(sendTelemetry(client_fd)==-1) break;
//         if(recieveCommands(client_fd)==-1) break;
//         sleep(2);
//     }

//     close(client_fd);
//     close(server_fd);
//     return 0;
// }
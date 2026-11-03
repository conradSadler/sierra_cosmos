#include "tcp_con.h"

static struct sockaddr_in server_addr;
static struct sockaddr_in client_addr;

/*
    Returns 0 if successful
    Returns -1 if failure came up
*/
uint8_t init_con(
    int *server_fd, 
    int *client_fd,
    uint16_t port){

    socklen_t addr_len = sizeof(client_addr);
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
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bounding to a local address with bind
    if (bind(*server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
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

    // accept() blocks until a client connects
    if((*client_fd = accept(*server_fd, (struct sockaddr*)&client_addr, &addr_len)) < 0){
        perror("Accept failed");
        close(*server_fd);
        return -1;
    }
    int flags = fcntl(*client_fd, F_GETFL, 0);
    // Open client connection with file control to 
    // Use a bitwise or to add the O_NONBLOCK flag to the sockets current flags
    fcntl(*client_fd, F_SETFL, flags | O_NONBLOCK);

    // ntohs - converts the unsigned integer netlong from network byte order to host byte order.
    printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return 0;
}

uint8_t sendTelemetry(int client_fd){
    uint32_t cpu_temp;
    
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp == NULL){
        perror("Couldn't get file!");
        return -1;
    }
    int items_matched = fscanf(fp, "%d", &cpu_temp);
    fclose(fp);
    if(items_matched != 1){
        perror("Reading File Error");
        return -1;
    }
    printf("%x", cpu_temp);
    // htons converts a short(2-byte) integer from local host order to network byte order
    // (0x3 << 14) shift 11 for unsegmented to the first 2 bits of seq_ctrl int
    // (seq_count++ & 0x3FFF) increment sequence count and use bit mask to ensure that it does not overflow
    payload_t packet = {
        packet_id = htons(CCSDS_APID),
        seq_ctrl = htons((0x3 << 14) | (seq_count++ & 0x3FFF)),
        length = htons(sizeof(int32_t) - 1),
        cpu_temp = htonl(cpu_temp),
    };

    if(send(client_fd, &packet, MSG_NOSIGNAL) < 0){
        perror("Sending bytes error!");
        return -1;
    }
    return 0;
}

uint8_t receive_commands(int client_fd){
    uint8_t buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
    if(bytes_read < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK ){
            return 0;
        }
        perror("Read failed");
        return -1;
    }
    else if (bytes_read == 0){
        printf("%s","Disconnected");
        return -1;
    }
    printf("Received %zd bytes: %x\n", bytes_read, buffer);
    return 0;
}

int main(){
    // File Descriptors
    int server_fd, client_fd;

    if(init_con(&server_fd, &client_fd, PORT) == -1){
        printf("init_con error");
        exit(EXIT_FAILURE);
    }

    while(1){
        if(sendTelemetry(client_fd)==-1) break;
        if(receive_commands(client_fd)==-1) break;
        sleep(2);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
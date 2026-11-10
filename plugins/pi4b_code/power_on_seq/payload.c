#include "payload.h"

// static struct sockaddr_in rack_addr;
static struct sockaddr_in host_addr;
// static struct sockaddr_in payload_addr;

int main(){
    memset(&host_addr, 0, sizeof(host_addr));
    // uint32_t rack_server_fd, rack_client_fd;                       //File Descriptor
    int32_t host_fd;                       //File Descriptor
    // int sock = 0;

    if ((host_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(HOST_PORT);

    if (inet_pton(AF_INET, "192.168.84.2", &host_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    if (connect(host_fd, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }


    // if(init_con(&rack_server_fd, &rack_client_fd, &rack_addr, &host_addr, RACK_PORT) == -1){
    //     printf("init_con error on connecting to rack");
    //     exit(EXIT_FAILURE);
    // }


    // if(init_con(&host_server_fd, &host_client_fd, &payload_addr, &host_addr, HOST_PORT) == -1){
    //     printf("init_con error on connecting to host");
    //     exit(EXIT_FAILURE);
    // }

    while(1){
        // if(send_telemetry(host_client_fd)==-1) break;
        if(payload_receive_message(host_fd)==-1) break;
        // sleep(2);
    }

    // close(rack_server_fd);
    // close(rack_client_fd);
    close(host_fd);
    // close(host_client_fd);
    return 0;
}
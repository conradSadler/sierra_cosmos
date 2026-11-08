#include "payload.h"

static struct sockaddr_in rack_addr;
static struct sockaddr_in host_addr;
static struct sockaddr_in payload_addr;

int main(){
    // uint32_t rack_server_fd, rack_client_fd;                       //File Descriptor
    uint32_t host_server_fd, host_client_fd;                       //File Descriptor

    // if(init_con(&rack_server_fd, &rack_client_fd, &rack_addr, &host_addr, RACK_PORT) == -1){
    //     printf("init_con error on connecting to rack");
    //     exit(EXIT_FAILURE);
    // }

    if(init_con(&host_server_fd, &host_client_fd, &payload_addr, &host_addr, HOST_PORT) == -1){
        printf("init_con error on connecting to host");
        exit(EXIT_FAILURE);
    }

    while(1){
        if(sendTelemetry(host_client_fd)==-1) break;
        if(recieveCommands(host_client_fd)==-1) break;
        sleep(2);
    }

    // close(rack_server_fd);
    // close(rack_client_fd);
    close(host_server_fd);
    close(host_client_fd);
    return 0;
}
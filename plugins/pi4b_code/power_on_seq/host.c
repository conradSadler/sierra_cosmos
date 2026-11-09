#include "host.h"

static struct sockaddr_in rack_addr;
static struct sockaddr_in host_addr;
static struct sockaddr_in payload_addr;

int main(){
    int32_t rack_server_fd, rack_client_fd;                       //File Descriptor
    int32_t payload_server_fd, payload_client_fd;                       //File Descriptor

    // if(init_con(&rack_server_fd, &rack_client_fd, &rack_addr, &host_addr, RACK_PORT) == -1){
    if(init_con(&rack_server_fd, &rack_client_fd, &host_addr, &rack_addr, RACK_PORT) == -1){
        printf("init_con error on connecting to rack");
        exit(EXIT_FAILURE);
    }

    // if(init_con(&payload_server_fd, &payload_client_fd, &payload_addr, &host_addr, PAYLOAD_PORT) == -1){
    if(init_con(&payload_server_fd, &payload_client_fd, &host_addr, &payload_addr, PAYLOAD_PORT) == -1){
        printf("init_con error on connecting to payload");
        exit(EXIT_FAILURE);
    }

    while(1){
        if(send_telemetry(rack_client_fd)==-1) break;
        if(host_send_message(payload_client_fd, MESSAGE_ID_POWER_ON)==-1) break;
        if(receive_commands(rack_client_fd)==-1) break;
        sleep(2);
    }

    close(rack_server_fd);
    close(rack_client_fd);
    close(payload_server_fd);
    close(payload_client_fd);
    return 0;
}

#include "tcp_con.h"

// static struct sockaddr_in server_addr;
// static struct sockaddr_in client_addr;
uint16_t seq_count = 0;

/*
    Returns 0 if successful
    Returns -1 if failure came up
*/
int32_t init_con(
    int32_t *server_fd, 
    int32_t *client_fd,
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
    int32_t flags = fcntl(*client_fd, F_GETFL, 0);
    fcntl(*client_fd, F_SETFL, flags | O_NONBLOCK);

    printf("Connection accepted from %s:%d\n", 
        inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    return 0;
}

// INCOMPLETE
/* 
   This function handles the receiving of messages 
    between host and payload (not commands)
*/
int32_t payload_receive_message(int32_t client_fd){
    payload_t payload;
    ssize_t bytes_read = recv(client_fd, &payload, sizeof(payload_t)-1, 0);
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
    printf("Received %zd bytes\n", bytes_read);
    printf("%x\n", payload);
    printf("%x\n", (payload.header.packet_identification));
    printf("%x\n", (payload.header.packet_sequence_control));
    printf("%x\n", (payload.header.packet_data_length));
    printf("%x\n", (payload.data));
    switch (payload.header.packet_identification & APID_BIT_MASK){
        case MESSAGE_ID_POWER_ON:
            // Nothing to do as no need to typecast
            printf("In message: power_on\n");
            break;
        case MESSAGE_ID_ID_EXCHANCE:
            printf("In message: id_exchange\n");
            // id_exchange_message_t *data = (id_exchange_message_t*)payload.data;
            break;
        case MESSAGE_ID_TIME:
            printf("In message: time\n");
            // time_message_t *data = (time_message_t*) payload.data;
            break;
        case MESSAGE_ID_STATE:
            printf("In message: state\n");
            // state_message_t *data = (state_message_t*) payload.data;
            break;
        case MESSAGE_ID_OFFSET:
            printf("In message: offset\n");
            // offset_message_t *data = (offset_message_t*) payload.data;
            break;
        case MESSAGE_ID_DO_BIT:
            printf("In message: do_bit\n");
            // do_bit_message_t *data = (do_bit_message_t*) payload.data;
            break;
        case MESSAGE_ID_BIT_COMPLETE_STATUS:
            printf("In message: bit_complete_status\n");
            // bit_complete_status_message_t *data = (bit_complete_status_message_t*) payload.data;
            break;
        case MESSAGE_ID_TELEMETRY_UPDATE:
            printf("In message: telemetry_update\n");
            // telemetry_update_message_t *data = (telemetry_update_message_t*) payload.data;
            break;
        // ...
        default:
            perror("OOB message!");
            return -1;
            // break;
    }
    return 0;
}

int32_t host_receive_message(int32_t client_fd){
    // switch (payload.header.packet_identification & APID_BIT_MASK){
    //     case MESSAGE_ID_POWER_ON:
    //         // Nothing to do as no need to typecast 
    //         break;
    //     case MESSAGE_ID_ID_EXCHANCE:
    //         id_exchange_message_t *data = (id_exchange_message_t*)payload.data;
    //         break;
    //     case MESSAGE_ID_TIME:
    //         time_message_t *data = (time_message_t*) payload.data;
    //         break;
    //     case MESSAGE_ID_STATE:
    //         state_message_t *data = (state_message_t*) payload.data;
    //         break;
    //     case MESSAGE_ID_OFFSET:
    //         offset_message_t *data = (offset_message_t*) payload.data;
    //         break;
    //     case MESSAGE_ID_DO_BIT:
    //         do_bit_message_t *data = (do_bit_message_t*) payload.data;
    //         break;
    //     case MESSAGE_ID_BIT_COMPLETE_STATUS:
    //         bit_complete_status_message_t *data = (bit_complete_status_message_t*) payload.data;
    //         break;
    //     case MESSAGE_ID_TELEMETRY_UPDATE:
    //         telemerty_update_message_t *data = (telemerty_update_message_t*) payload.data;
    //         break;
    //     // ...
    //     default:
    //         perror("OOB message!");
    //         return -1;
    //         // break;
    // }
    return 0;
}

int32_t host_send_message(int32_t client_fd, uint16_t message_id){
    uint32_t data_length = 0;
    // payload_t payload = {
    //     .header.packet_identification = htons(FIRST_TWO_OCTET(6, 1, 0, 1)),
    //     .header.packet_sequence_control = htons(SECOND_TWO_OCTET(message_id, seq_count++)),
    //     .header.packet_data_length = htons(data_length),
    //     .data = (void*)htonl(0x21)
    // };
    payload_t payload = {
        .header.packet_identification = FIRST_TWO_OCTET(6, 1, 0, 1),
        .header.packet_sequence_control = SECOND_TWO_OCTET(message_id, seq_count++),
        .header.packet_data_length = data_length,
        .data = (void*)0x21
    };
    printf("%x\n", payload);
    printf("%x\n", (payload.header.packet_identification));
    printf("%x\n", (payload.header.packet_sequence_control));
    printf("%x\n", (payload.header.packet_data_length));
    printf("%x\n", (payload.data));
    ssize_t bytes_sent = send(client_fd, &payload, sizeof(payload), MSG_NOSIGNAL);
    if(bytes_sent != (ssize_t)sizeof(payload)){
        if(bytes_sent < 0){
            perror("Error sending bytes!\n");
        }
        else{
            fprintf(stderr, "Error sending bytes: short write\n");
        }
        return -1;
    }
    return 0;
}

int32_t send_telemetry(int32_t client_fd){
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


    payload_t packet = {
        .header.packet_identification = htons(FIRST_TWO_OCTET(6, 1, 0, 1)),
        .header.packet_sequence_control = htons(SECOND_TWO_OCTET(1, seq_count++)),
        .header.packet_data_length = htons(sizeof(int32_t)-1),
        .data = (void*)htonl(cpu_temp)
    };
    // //printf("size: %zu, %b\n", sizeof(packet), packet);
    // printf("size: %zu, hdr: %04x %04x %04x %08x\n", sizeof(packet),
    // //printf("size: %zu, hdr: %16b %16b %16b %04x\n", sizeof(packet),
    //    ntohs(packet.header.packet_identification), ntohs(packet.header.packet_sequence_control),
    //    ntohs(packet.header.packet_data_length), ntohl((uint32_t)packet.data));
    ssize_t bytes_sent = send(client_fd, &packet, sizeof(packet), MSG_NOSIGNAL);
    if(bytes_sent != (ssize_t)sizeof(packet)){
        if(bytes_sent < 0){
            perror("Error sending bytes!\n");
        }
        else{
            fprintf(stderr, "Error sending bytes: short write\n");
        }
        return -1;
    }
    return 0;
}

int32_t receive_commands(int32_t client_fd){
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
    printf("Received %zd bytes\n", bytes_read);
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

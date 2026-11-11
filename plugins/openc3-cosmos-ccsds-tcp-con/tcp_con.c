#include "tcp_con.h"
extern char **environ;

static struct sockaddr_in server_addr_tlm;
static struct sockaddr_in server_addr_cmd;
static struct sockaddr_in client_addr;
int seq_count = 0;

/*
    Returns 0 if successful
    Returns -1 if failure came up
*/
int init_con(
    int *server_fd_tlm, 
    int *server_fd_cmd, 
    int *client_fd_tlm,
    int *client_fd_cmd,
    uint16_t port_1,
    uint16_t port_2){

    socklen_t addr_len = sizeof(client_addr);
    int opt = 1;
    
    // Create a socket
    if((*server_fd_tlm = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket creation failed");
        return -1;
    }
    if(setsockopt(*server_fd_tlm, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt failed");
        close(*server_fd_tlm);
        return -1;
    }

    // Create a socket
    if((*server_fd_cmd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket creation failed");
        return -1;
    }
    if(setsockopt(*server_fd_cmd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt failed");
        close(*server_fd_cmd);
        return -1;
    }

    server_addr_tlm.sin_family = AF_INET;
    server_addr_cmd.sin_family = AF_INET;
    server_addr_tlm.sin_addr.s_addr = INADDR_ANY;
    server_addr_cmd.sin_addr.s_addr = INADDR_ANY;
    server_addr_tlm.sin_port = htons(port_1);
    server_addr_cmd.sin_port = htons(port_2);

    // Binding to a local address
    if (bind(*server_fd_tlm, (struct sockaddr*)&server_addr_tlm, sizeof(server_addr_tlm)) < 0){
        perror("Bind failed");
        close(*server_fd_tlm);
        return -1;
    }

    // Binding to a local address
    if (bind(*server_fd_cmd, (struct sockaddr*)&server_addr_cmd, sizeof(server_addr_cmd)) < 0){
        perror("Bind failed");
        close(*server_fd_cmd);
        return -1;
    }

    // Listen with queue limit of 3
    // https://man7.org/linux/man-pages/man2/listen.2.html
    if (listen(*server_fd_tlm, 3) < 0){
        perror("Listen failed");
        close(*server_fd_tlm);
        return -1;
    }

    if (listen(*server_fd_cmd, 3) < 0){
        perror("Listen failed");
        close(*server_fd_cmd);
        return -1;
    }

    printf("Server listening on %d...\n", port_1);
    printf("Server listening on %d...\n", port_2);


    // accept() blocks until a client connects
    if((*client_fd_tlm = accept(*server_fd_tlm, (struct sockaddr*)&client_addr, &addr_len)) < 0){
        perror("Accept failed");
        close(*server_fd_tlm);
        return -1;
    }
    // accept() blocks until a client connects
    if((*client_fd_cmd = accept(*server_fd_cmd, (struct sockaddr*)&client_addr, &addr_len)) < 0){
        perror("Accept failed");
        close(*server_fd_cmd);
        return -1;
    }
    int flags_tlm = fcntl(*client_fd_tlm, F_GETFL, 0);
    int flags_cmd = fcntl(*client_fd_cmd, F_GETFL, 0);
    // Open client connection with file control to 
    // Use a bitwise or to add the O_NONBLOCK flag to the sockets current flags
    fcntl(*client_fd_tlm, F_SETFL, flags_tlm | O_NONBLOCK);
    fcntl(*client_fd_cmd, F_SETFL, flags_cmd | O_NONBLOCK);

    // ntohs - converts the unsigned integer netlong from network byte order to host byte order.
    //printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    //printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return 0;
}

int send_telemetry(int client_fd_tlm){
    uint32_t cpu_temp;
    
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp == NULL){
        perror("Couldn't get file!");
        return -1;
    }
    int items_matched = fscanf(fp, "%" SCNu32, &cpu_temp);
    fclose(fp);
    if(items_matched != 1){
        perror("Reading File Error");
        return -1;
    }
    // htons converts a short(2-byte) integer from local host order to network byte order
    // (0x3 << 14) shift 11 for unsegmented to the first 2 bits of seq_ctrl int
    // (seq_count++ & 0x3FFF) increment sequence count and use bit mask to ensure that it does not overflow
    payload_t packet = {
        .packet_id = htons(CCSDS_APID),
        .seq_ctrl = htons((0x3 << 14) | (seq_count++ & 0x3FFF)),
        .length = htons(sizeof(int32_t) - 1),
        .cpu_temp = htonl(cpu_temp),
    };
    ssize_t sent_total = 0;
    ssize_t packet_size=sizeof(packet);
    while ( sent_total < packet_size )
    {
        ssize_t bytes_sent = send(client_fd_tlm,((uint8_t *)&packet) + sent_total,packet_size - sent_total, MSG_NOSIGNAL);
        if (bytes_sent < 0)
        {   
            // TCP connection was full so try again
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Skip the remaining loop if the TCP connection is full/busy
                continue;
            }
            else
            {
                perror("Error sending bytes!");
                return -1;
            }

        }
        if (bytes_sent == 0)
        {
            fprintf(stderr, "Error sending bytes: connection closed\n");
            return -1;
        }
        sent_total += bytes_sent;
        
    }
    return 0;
}

int receive_commands(int client_fd_cmd){
    // buffer is 8 bit integers to keep reading packet payload simple
    // This causes packet headers feilds to be split
    // buffer[0] -> [ version (3 bits), type (1 bit), Secondary header (1 bit), APID (first 3 bits most significant bits) ]
    // buffer[1] -> [ APID (last 8 bits) ]
    // buffer[2] -> [ Sequence flag (2 bits), Sequence count (first 6 most significant bits)]
    // buffer[3] -> [ Sequence count (last 8 bits) ]
    // buffer[4] -> [ length (first 8 most significant bits) ]
    // buffer[5] -> [ length (last 8 bits)]
    uint8_t buffer[BUFFER_SIZE];
    int count = 0;
    // total bytes received
    int total = 0;
    // File pointer
    FILE *fptr;

    // While there are bytes from the client, continue to read them into buffer.
    // If the client disconnects or a read fails exit the loop and handle below
    while ((count = recv(client_fd_cmd, &buffer[total], BUFFER_SIZE - total, 0)) > 0)
    {
        total += count;
        // While buffer does not have the whole 6 byte packet header, continue reading and skip the information below
        if (total < 6) 
        {
             continue;
        }
        // Reconstruct the length field from the packet header
        uint16_t data_length = ((uint16_t) buffer[4] << 8) | buffer[5];
        // If a whole packet is received then proccess it
        if(total >= 6 + data_length)
        {
            // flag.txt wil be created if it does not already exist
            // If flag.txt already exists, then the file will be emptied before adding the next flag
            fptr = fopen("flag.txt", "w");
            if (fptr == NULL)
            {
                printf("Error: flag.txt was not able to be opened.");
                return -1;
            }
            char null_byte='\0';

            for(int i = 6; i < total; i++)
            {
                fprintf(fptr,"%c",(char)buffer[i]); // Add a character every iteration
            }
            fprintf(fptr,"%c",null_byte); // Add the null byte at the end becasue fprintf does not do that
            fclose(fptr);
            return 0;
        }
        else
        {
            printf("Packet lenght is: %d, not the expected length of: %d\n", total, 6 + data_length);
        }
    }

    if(count < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK ){
            return 0;
        }
        perror("Read failed");
        return -1;
    }
    else if (count == 0)
    {
        // EOS on the socket: close it, exit the thread
        printf("%s","Disconnected");
        return -1;
    }
    else
    {
        return 0;
    }
}

int main(){
    // File Descriptors
    int server_fd_tlm, server_fd_cmd, client_fd_tlm, client_fd_cmd;

    if(init_con(&server_fd_tlm,&server_fd_cmd, &client_fd_tlm,&client_fd_cmd, PORT,8081)  == -1){
        printf("init_con error");
        exit(EXIT_FAILURE);
    }


    while(1){
        if(send_telemetry(client_fd_tlm)==-1) break;
        if(receive_commands(client_fd_cmd)==-1) break;
        sleep(.01);
    }

    close(client_fd_tlm);
    close(client_fd_cmd);
    close(server_fd_tlm);
    close(server_fd_cmd);
    return 0;
}

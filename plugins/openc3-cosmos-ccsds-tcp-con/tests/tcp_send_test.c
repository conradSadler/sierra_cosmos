#include "../tcp_con.h"
#include <assert.h>

static const int send_results[] = {-EAGAIN, 3, -EINTR, -EWOULDBLOCK, 7, 10, -EPIPE, 0};
static size_t send_calls;
static size_t sample_reads;
static uint8_t received[20];
static size_t received_size;

static ssize_t scripted_send(int fd, const void *buffer, size_t length, int flags){
    (void)fd;
    assert(flags == MSG_NOSIGNAL);
    assert(send_calls < sizeof(send_results) / sizeof(send_results[0]));
    int result = send_results[send_calls++];
    if(result < 0){
        errno = -result;
        return -1;
    }
    assert((size_t)result <= length);
    assert(received_size + (size_t)result <= sizeof(received));
    memcpy(received + received_size, buffer, (size_t)result);
    received_size += (size_t)result;
    return result;
}

static FILE *temperature_file(const char *path, const char *mode){
    assert(strcmp(path, "/sys/class/thermal/thermal_zone0/temp") == 0);
    assert(strcmp(mode, "r") == 0);
    FILE *file = tmpfile();
    assert(file != NULL);
    fputs("42000\n", file);
    rewind(file);
    sample_reads++;
    return file;
}

/* Exercise the production send path with deterministic socket backpressure. */
#define send scripted_send
#define fopen temperature_file
#define main tcp_server_main
#include "../tcp_con.c"
#undef main
#undef fopen
#undef send

int main(void){
    assert(sizeof(payload_t) == 10);
    assert(send_telemetry(1) == 0); /* Full buffer before any bytes accepted. */
    assert(received_size == 0 && send_calls == 1);
    assert(sample_reads == 1 && seq_count == 1);

    assert(send_telemetry(1) == 0); /* Partial write, interrupt, then full buffer. */
    assert(received_size == 3 && send_calls == 4);
    assert(sample_reads == 1 && seq_count == 1);

    assert(send_telemetry(1) == 0); /* Resume the same frame. */
    assert(received_size == 10 && sample_reads == 1 && seq_count == 1);
    assert(send_telemetry(1) == 0); /* Only now generate the next sample. */
    assert(received_size == 20 && sample_reads == 2 && seq_count == 2);

    const uint8_t expected[] = {
        0x00, 0x01, 0xc0, 0x00, 0x00, 0x03, 0x00, 0x00, 0xa4, 0x10,
        0x00, 0x01, 0xc0, 0x01, 0x00, 0x03, 0x00, 0x00, 0xa4, 0x10
    };
    assert(memcmp(received, expected, sizeof(expected)) == 0);

    size_t offset = 0;
    assert(flush_packet(1, expected, 10, &offset) == -1); /* Closed peer. */
    assert(offset == 0 && errno == EPIPE);
    assert(flush_packet(1, expected, 10, &offset) == -1); /* No forward progress. */
    assert(offset == 0 && errno == EPIPE);
    puts("TCP send tests passed (two expected broken-pipe diagnostics above)");
    return 0;
}

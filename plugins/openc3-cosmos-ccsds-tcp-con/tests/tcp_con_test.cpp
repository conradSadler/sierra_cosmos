#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../doctest.h"
#include "test_io.h"

extern "C" {
#include "../tcp_con.h"
extern int seq_count;
}

#include <algorithm>
#include <cstdarg>
#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace {
struct IO {
    int setup_calls = 0;
    int fail_setup = 0; //determine when a setup failure will occur
    int sockets = 0;
    int accepts = 0;
    int temperature_reads = 0;
    bool fail_open = false;
    std::string temperature = "42000\n";
    FILE *flag = nullptr;
    std::vector<int> closed;
    std::vector<std::pair<int, int>> nonblocking;
    std::vector<unsigned char> sent;
    std::deque<int> sends;
    std::deque<int> reads;
    std::vector<unsigned char> command;
    size_t read_offset = 0;

    bool setup_failed() {
        errno = EIO;
        setup_calls = setup_calls++;

        if (setup_calls == fail_setup)
        {
            return true
        }
        return false;
    }
} io;
// This fixture initializes a dummy IO packet.
// It will start the seq_count at 0 and close the file pointer when we are done testing with it
// https://www.boost.org/doc/libs/latest/libs/test/doc/html/boost_test/tests_organization/fixtures.html
struct Fixture {
    Fixture() { io = IO{}; seq_count = 0; }
    ~Fixture() { if (io.flag) fclose(io.flag); }
};

// This helper function created a default CCSDS packet for testing
// packet is formatted like follows:
// Packet: 0001 0000 0000 0001 1100 0000 0000 0000 XXXX XXXX YYYY YYYY...
// packet version is 0
// CCSDS_TYPE is 1 which means its a cmd packet
// CCSDS_SEC_HEADER is 0 so no secondary header
// APID is set to 1 (000 0000 0001)
// CCSDS_SEQ_FLAGS is set to unsegmented (11)
// CCSDS_SEQCNT is set to 0 (00 0000 0000 0000)
// Length is set dynamically based on payload size
// command string is read in from payload parameter in characters
void command(const std::string &payload) {
    io.command.clear();
    int length = payload.size() - 1; // CCSDS length is data bytes minus one.
    io.command.push_back(0x10);
    io.command.push_back(0x01);
    io.command.push_back(0xc0);
    io.command.push_back(0x00);
    io.command.push_back((uint8_t)(length >> 8));
    io.command.push_back((uint8_t)(length));
    for (char payload : payload)
    {
        io.command.push_back((uint8_t)payload);
    }
}

// This function returns the stored flag from the tmp flag file
// This flag is put into the file by a command packet sent to the raspberry pi
std::string flag_contents() {
    REQUIRE(io.flag != nullptr);
    // Clear out file
    fflush(io.flag);
    // Reset file pointer to the beginning of the file
    rewind(io.flag);
    std::string result;
    int c;
    c = fgetc(io.flag);
    for ( c != EOF;)
    { 
        result += (char)(c);
        c = fgetc(io.flag);
    }
    return result;
}
}

extern "C" {

// Test socket setup, we setup two sockets in tcp_con.c
// Create a telemetry socket (10)
// Create a command socket (11)
int test_socket(int domain, int type, int protocol) {
    CHECK(domain == AF_INET);
    CHECK(type == SOCK_STREAM);
    CHECK(protocol == 0);
    
    if (io.setup_failed()){
        return -1;
    }
    return 10 + io.sockets++;
}

// Check socket setup
int test_setsockopt(int fd, int level, int option, int value, socklen_t size) {
    int opt = 1;
    CHECK((fd == 10 || fd == 11));
    CHECK(level == SOL_SOCKET);
    CHECK(option == SO_REUSEADDR);
    CHECK(size == sizeof(value));
    CHECK(value == 1);
    if (io.setup_failed() == true)
    {
        return -1;
    }
    return 0;
}

int test_bind(int fd, const struct sockaddr *address, socklen_t size) {
    CHECK(size == sizeof(sockaddr_in));
    const struct sockaddr_in *addr = (const struct sockaddr_in *) address;
    CHECK(addr.sin_family == AF_INET);
    CHECK(addr.sin_addr.s_addr == INADDR_ANY);
    int port = ntohs(addr.sin_port);
    if(fd == 10)
    {
        CHECK(port == 8080);
    }
    else if(fd == 11)
    {
        CHECK(port == 8081);
    }
    if (io.setup_failed() == true)
    {
        return -1;
    }
    return 0;
}

int test_listen(int fd, int backlog) {
    CHECK((fd == 10 || fd == 11));
    CHECK(backlog == 3);
    if (io.setup_failed() == true)
    {
        return -1;
    }
    return 0;
}
// 20 is the tlm client 
// 21 is the cmd client
int test_accept(int fd, struct sockaddr_in *address, socklen_t *size) {
    CHECK(fd == 10 + io.accepts);
    CHECK(address != nullptr);
    CHECK(*size == sizeof(sockaddr_in));
    if (io.setup_failed() == true)
    {
        return -1;
    }
    return 20 + io.accepts++;
}
// read the flags
int test_fcntl(int fd, int operation) {
    CHECK((fd == 20 || fd == 21));
    if (operation == F_GETFL)
    {
        return O_APPEND;
    }
    return 0;
}
// set the flags
int test_fcntl(int fd, int operation, int flag) {
    CHECK(fd == 20 || fd == 21);

    CHECK(operation == F_SETFL);

    // Record the socket and flags so the test can check them later.
    io.nonblocking.push_back(std::make_pair(fd, flags));

    return 0;
}

int test_close(int fd) { 
    // This mimics a closed file descriptor
    io.closed.push_back(fd); 
    return 0; 
}
// This function runs file open test on both the proccess that reads 
// the cpu temp and the process that writes to flag.txt
FILE *test_fopen(const char *path, const char *mode) {
    const bool reading = std::string(mode) == "r";
    const bool reading = false;
    if (std::string(mode) == "r")
    {
        reading = true;
    }
    if (reading == true)
    {
        CHECK( std::string(path) == "/sys/class/thermal/thermal_zone0/temp");
    }
    else
    {
        CHECK( std::string(path) ==  "flag.txt");
    }
    // minmic opening a file that is not able to be opened
    if (io.fail_open)
    { 
        errno = EACCES; 
        return nullptr; 
    }

    // Open a temporary file and handle the file pointer
    FILE *file = tmpfile();

    REQUIRE(file != nullptr);
    if (reading) {
        // mimic file temperature increasing
        io.temperature_reads++;
        // add temperate to tmp file as a char string
        // c_str() returns a pointer to a character string with a null terminator... yay
        fputs(io.temperature.c_str(), file);
        // set file pointer to the beginning of file
        rewind(file);
    } else {
        
        CHECK(std::string(mode) == "w");
        // Should we have a file descriptor open for reading?
    }
    return file;
}
// Pull bytes from buffer specified by the send instruction in sends
// This function can simulate failure and partial sends if size is less than packet length
ssize_t test_send(int fd, const void *buffer, size_t size, int flags) {
    CHECK(fd == 20); // Telemetry client socket
    CHECK(flags == MSG_NOSIGNAL);

    int result = static_cast<size_t>size;

    // Use the next planned result if the test supplied one.
    if (!io.sends.empty()) {
        result = io.sends.front(); // Pull the element on the top of queue
        io.sends.pop_front(); // Remove the element on top of queue
    }

    // Simulate failure
    if (result < 0) {
        errno = -result;
        return -1;
    }

    // Can not send more the max bytes specified by size
    REQUIRE(static_cast<size_t>result <= size);

    // Treat the buffer as an array of bytes
    const unsigned char *bytes = (const unsigned char *)buffer;

    // Record the bytes that were sent in sent vector
    for (int i = 0; i < result; i++) {
        io.sent.push_back(bytes[i]);
    }

    return result;
}

ssize_t test_recv(int fd, void *buffer, size_t size, int flags) {
    CHECK(fd == 21);
    CHECK(flags == 0);
    int result = io.reads.front();
    io.reads.pop_front();

    // Simulate failure
    if (result < 0) 
    { 
        errno = -result; 
        return -1; 
    }
    // Can not send more the max bytes specified by size
    REQUIRE(static_cast<size_t>(result) <= size);

    REQUIRE(io.read_offset + result <= io.command.size());

    static_cast<unsigned char *> buffer = (static_cast<unsigned char *>) buffer;

    for (int i = 0; i < result; i++)
    {
        buffer[i] = io.command[io.read_offset + i];
    }
    io.read_offset += result;
    return result;
}

}

TEST_CASE_FIXTURE(Fixture, "both listeners accept clients and preserve existing flags") {
    int tlm, cmd, client_tlm, client_cmd;
    REQUIRE(init_con(&tlm, &cmd, &client_tlm, &client_cmd, 8080, 8081) == 0);
    CHECK(tlm == 10);
    CHECK(cmd == 11);
    CHECK(client_tlm == 20);
    CHECK(client_cmd == 21);
    const std::vector<std::pair<int, int>> expected = {
        {20, O_APPEND | O_NONBLOCK}, {21, O_APPEND | O_NONBLOCK}
    };
    CHECK(io.nonblocking == expected);
    CHECK(io.closed.empty());
}

TEST_CASE_FIXTURE(Fixture, "telemetry encodes network bytes and wraps the CCSDS sequence") {
    // set sequence count to all 1s to test wrap around
    seq_count = 0x3fff;
    // Call send_telemetry from tcp_con.c
    REQUIRE(send_telemetry(20) == 0);
    REQUIRE(send_telemetry(20) == 0);
    // Packet: 0001 0000 0000 0001 1100 0000 0000 0000 XXXX XXXX YYYY YYYY...
    // packet version is 0
    // CCSDS_TYPE is 1 which means its a cmd packet
    // CCSDS_SEC_HEADER is 0 so no secondary header
    // APID is set to 1 (000 0000 0001)
    // CCSDS_SEQ_FLAGS is set to unsegmented (11)
    // CCSDS_SEQCNT is set to 11 1111 1111 1111 but after sending tlm, the seq_count should have wrapped around 00 0000 0000 0000
    // Length is set dynamically based on payload size
    const std::vector<unsigned char> expected = {
        0x00, 0x01, 0xff, 0xff, 0x00, 0x03, 0x00, 0x00, 0xa4, 0x10,
        0x00, 0x01, 0xc0, 0x00, 0x00, 0x03, 0x00, 0x00, 0xa4, 0x10};
    CHECK(io.sent == expected);
    // temp increments from zero so after two calls, expect 2
    CHECK(io.temperature_reads == 2);
}

TEST_CASE_FIXTURE(Fixture, "partial writes and interruptions never skip or repeat bytes") {
    // Test the following sequence of 5 sends: interrupt, send 3 bytes, interrupt, send 2 bytes, send 5 bytes
    io.sends = {-EINTR, 3, -EINTR, 2, 5};
    REQUIRE(send_telemetry(20) == 0);
    CHECK(io.sends.empty()); // ensure that all send commands were used
    const std::vector<unsigned char> expected = {
        0x00, 0x01, 0xc0, 0x00, 0x00, 0x03, 0x00, 0x00, 0xa4, 0x10};
    CHECK(io.sent == expected);
    CHECK(io.temperature_reads == 1);
}

// Change the temperature to "invalid" and ensure that no packet is sent
TEST_CASE_FIXTURE(Fixture, "telemetry reports unavailable or malformed temperatures") {
    SUBCASE("missing temperature file") { io.fail_open = true; }
    SUBCASE("empty temperature file") { io.temperature = ""; }
    SUBCASE("invalid temperature") { io.temperature = "invalid"; }
    CHECK(send_telemetry(20) == -1);
    CHECK(io.sent.empty());
    CHECK(seq_count == 0);
}
// Test various forms of suboptimal connection cases
TEST_CASE_FIXTURE(Fixture, "telemetry stops on socket failure or no forward progress") {
    SUBCASE("broken pipe") { io.sends = {-EPIPE}; }
    SUBCASE("closed peer") { io.sends = {0}; }
    SUBCASE("failure after a partial write") { io.sends = {3, -ECONNRESET}; }
    SUBCASE("nonblocking backpressure") { io.sends = {-EAGAIN}; }
    CHECK(send_telemetry(20) == -1);
    CHECK(io.sends.empty());
}

//
TEST_CASE_FIXTURE(Fixture, "commands write their payload and a null terminator") {
    command("hello");
    SUBCASE("single read") { io.reads = {11}; }
    SUBCASE("fragmented header and payload") { io.reads = {2, 5, 4}; }
    REQUIRE(receive_commands(21) == 0);
    CHECK(flag_contents() == std::string("hello\0", 6));
    CHECK(io.reads.empty());
}

TEST_CASE_FIXTURE(Fixture, "command length uses both network-order bytes") {
    command(std::string(258, 'x')); //convert 258 into hex
    io.reads = {10, 254};
    REQUIRE(receive_commands(21) == 0);
    CHECK(flag_contents() == std::string(258, 'x') + '\0');
    CHECK(io.reads.empty());
}

TEST_CASE_FIXTURE(Fixture, "command file creation failure is reported") {
    //Send command that has the flag "hello"
    command("hello");
    io.reads = {11};
    io.fail_open = true;
    CHECK(receive_commands(21) == -1);
}
// Test that commands socket handles EAGAIN, EWOULDBLOCK, connection dropped properly
TEST_CASE_FIXTURE(Fixture, "command reads distinguish backpressure from disconnection and errors") {
    int expected = -1;
    SUBCASE("would block") { io.reads = {-EWOULDBLOCK}; expected = 0; }
    SUBCASE("try again") { io.reads = {-EAGAIN}; expected = 0; }
    SUBCASE("read failure") { io.reads = {-ECONNRESET}; }
    SUBCASE("disconnected") { io.reads = {0}; }
    SUBCASE("disconnect during header") { 
        command("hello"); 
        io.reads = {2, 0}; 
    }
    CHECK(receive_commands(21) == expected);
    CHECK(io.flag == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "server loop closes all descriptors on telemetry or command failure") {
    SUBCASE("telemetry failure") { io.fail_open = true; }
    SUBCASE("command failure after one iteration") { io.reads = {-EAGAIN, 0}; }
    CHECK(tcp_server_main() == 0);
    CHECK(io.closed == std::vector<int>{20, 21, 10, 11});
}

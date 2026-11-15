#include <gtest/gtest.h>
#include "test_io.h"

extern "C" {
#include "../tcp_con.h"
extern int seq_count;
}

#include <algorithm>
#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace {

// These are fake descriptors, not sockets opened by the operating system.
constexpr int telemetry_server = 10;
constexpr int command_server = 11;
constexpr int telemetry_client = 20;
constexpr int command_client = 21;

struct IO {
    int setup_calls = 0;
    // 0 == success
    int fail_setup = 0;
    int sockets = 0;
    int accepts = 0;
    int temperature_reads = 0;
    bool fail_open = false;
    std::string temperature = "42000\n";

    std::vector<int> closed;
    std::vector<std::pair<int, int>> nonblocking;
    std::vector<unsigned char> sent;
    std::deque<int> sends;
    std::deque<int> reads;
    std::vector<unsigned char> command;
    size_t read_offset = 0;

    std::string flag_contents;
    FILE *flag = nullptr;
    std::vector<FILE *> open_files;

    bool setup_failed() {
        setup_calls++;
        if (setup_calls == fail_setup) {
            errno = EIO;
            return true;
        }
        return false;
    }
};

IO io;

void reset() {
    for (FILE *file : io.open_files) {
        fclose(file);
    }
    io = IO{};
    errno = 0;
}
// First 6 bytes are CCSDS header stuff. Refer to cmd.txt for reference
void command(const std::string &payload) {
    ASSERT_FALSE(payload.empty());
    ASSERT_LE(payload.size(), 65536); //2**16
    size_t length = payload.size() - 1; // CCSDS length is data bytes minus one.
    io.command.clear();
    io.read_offset = 0;
    io.command.push_back(0x10); // Command packet, no secondary header.
    io.command.push_back(0x01); // APID 1.
    io.command.push_back(0xc0); // Unsegmented packet
    io.command.push_back(0x00); // sequence count 0
    // Set the length of the payload in the  
    io.command.push_back(static_cast<unsigned char>(length >> 8));
    io.command.push_back(static_cast<unsigned char>(length));
    for (char character : payload) {
        io.command.push_back(static_cast<unsigned char>(character));
    }
}

class TcpConnectionTest : public testing::Test {
protected:
    void SetUp() override {
        reset();
        seq_count = 0;
    }

    void TearDown() override {
        EXPECT_TRUE(io.open_files.empty()) << "Production left a temporary file open";
        reset();
    }
};

} // namespace

// These functions have C linkage because tcp_con.c calls them.
// ASSERT_* cannot return from a non-void function.
extern "C" {
// Expect socket(AF_INET, SOCK_STREAM, 0)
int test_socket(int domain, int type, int protocol) {
    EXPECT_EQ(domain, AF_INET);
    EXPECT_EQ(type, SOCK_STREAM);
    EXPECT_EQ(protocol, 0);
    if (io.setup_failed()) {
        ADD_FAILURE() << "Socket test failure triggered";
        return -1;
    }
    int fd = telemetry_server + io.sockets;
    io.sockets++;
    return fd;
}

int test_setsockopt(int fd, int level, int option, const void *value, socklen_t size) {
    // Only Allow a server tlm or cmd socket
    EXPECT_TRUE(fd == telemetry_server || fd == command_server);
    // only allow sockets
    EXPECT_EQ(level, SOL_SOCKET);
    // This socket option allows you to bind to a recently closed port
    EXPECT_EQ(option, SO_REUSEADDR);
    if (value == nullptr || size != sizeof(int)) {
        ADD_FAILURE() << "Expected an integer socket option";
        errno = EINVAL;
        return -1;
    }
    //Does value point to integer of 0? setsockopt returns 0 upon successful completion
    EXPECT_EQ(*static_cast<const int *>(value), 0);
    if (io.setup_failed()) {
        return -1;
    }
    return 0;
}

int test_bind(int fd, const struct sockaddr *address, socklen_t size) {
    // only accept server cmd or tlm socket
    EXPECT_TRUE(fd == telemetry_server || fd == command_server);
    if (address == nullptr || size != sizeof(sockaddr_in)) {
        ADD_FAILURE() << "Expected an IPv4 socket address";
        errno = EINVAL;
        return -1;
    }
    // Use of reinterpret_cast preferred to C style cast
    // This is convention: https://stackoverflow.com/questions/11684008/how-do-you-cast-sockaddr-structure-to-a-sockaddr-in-c-networking-sockets-ubu
    const sockaddr_in *addr = reinterpret_cast<const sockaddr_in *>(address);
    EXPECT_EQ(addr->sin_family, AF_INET);
    EXPECT_EQ(addr->sin_addr.s_addr, INADDR_ANY);
    if (fd == telemetry_server) {
        EXPECT_EQ(ntohs(addr->sin_port), 8080);
    } else {
        EXPECT_EQ(ntohs(addr->sin_port), 8081);
    }
    if (io.setup_failed()) {
        return -1;
    }
    return 0;
}

int test_listen(int fd, int backlog) {
    EXPECT_TRUE(fd == telemetry_server || fd == command_server);
    // The connection queue is 3 large for listener, so that is what we expect
    // listen(*server_fd_tlm, 3)
    EXPECT_EQ(backlog, 3);
    if (io.setup_failed()) {
        return -1;
    }
    return 0;
}

int test_accept(int fd, struct sockaddr *address, socklen_t *size) {
    if (address == nullptr || size == nullptr) {
        ADD_FAILURE() << "Expected an address and its size";
        errno = EINVAL;
        return -1;
    }
    EXPECT_EQ(*size, sizeof(sockaddr_in));
    if (io.setup_failed()) {
        return -1;
    }
    int client_fd = telemetry_client + io.accepts;
    io.accepts++;
    return client_fd;
}
// Test manipulate file descriptor function while 
int test_fcntl(int fd, int operation, int flags) {
    EXPECT_TRUE(fd == telemetry_client || fd == command_client);
    if (operation == F_GETFL) {
        return O_APPEND; // An existing flag that production must preserve.
    }
    EXPECT_EQ(operation, F_SETFL);
    // Make pair is a C++ template that I have not used before. It works as expected
    io.nonblocking.push_back(std::make_pair(fd, flags));
    return 0;
}

int test_close(int fd) {
    io.closed.push_back(fd);
    return 0;
}

// This function allows for the testing of both write and read file actions in tcp_con.c
FILE *test_fopen(const char *path, const char *mode) {
    bool reading = std::string(mode) == "r";
    if (reading) {
        EXPECT_STREQ(path, "/sys/class/thermal/thermal_zone0/temp");
    } else {
        EXPECT_STREQ(path, "flag.txt");
        EXPECT_STREQ(mode, "w");
    }
    // Simulate system denied write
    if (io.fail_open) {
        errno = EACCES;
        return nullptr;
    }

    FILE *file = tmpfile();
    if (file == nullptr) {
        ADD_FAILURE() << "Could not create a temporary file: errno=" << errno;
        return nullptr;
    }
    io.open_files.push_back(file);
    if (reading) {
        io.temperature_reads++;
        // fputs should return a non-negative value upon successful exit, after writing null terminated c string to temp file
        EXPECT_GE(fputs(io.temperature.c_str(), file), 0);
        // Reset file pointer to the begining of the file
        rewind(file);
    } else {
        // Set store fake flag.txt File pointer in IO's flag field
        io.flag = file;
    }
    return file;
}

int test_fclose(FILE *file) {
    auto entry = std::find(io.open_files.begin(), io.open_files.end(), file);
    if (entry == io.open_files.end()) {
        ADD_FAILURE() << "Attempted to close an unknown or already closed file";
        // Bad file descriptor
        errno = EBADF;
        return EOF;
    }
    if (file == io.flag) {
        // Save the output before really closing the file
        EXPECT_EQ(fflush(file), 0);
        // Reset file pointer to the begining of the file
        rewind(file);
        io.flag_contents.clear();
        int character = fgetc(file);
        while (character != EOF) {
            // Save the contents of the File into flag_contents character by character
            io.flag_contents.push_back(static_cast<char>(character));
            character = fgetc(file);
        }
        EXPECT_EQ(ferror(file), 0);
        io.flag = nullptr;
    }
    io.open_files.erase(entry);
    return fclose(file);
}

ssize_t test_send(int fd, const void *buffer, size_t size, int flags) {
    EXPECT_EQ(fd, telemetry_client);
    // Expect MSG_NOSIGNAL, this is a handy flag because it prevents the kernel from SIGPIPE when connection is closed
    // This lets us better handle that situation manually
    EXPECT_EQ(flags, MSG_NOSIGNAL);
    ssize_t result = static_cast<ssize_t>(size);
    if (!io.sends.empty()) {
        //Take the first item off the send queue to simulate sending data
        result = io.sends.front();
        io.sends.pop_front();
    }
    if (result < 0) {
        // Tac on a minus sign to make it an error code
        errno = static_cast<int>(-result);
        return -1;
    }
    // We should not be able to send more than max bytes specified by size
    if (static_cast<size_t>(result) > size) {
        ADD_FAILURE() << "Planned send exceeds the supplied buffer";
        errno = EIO;
        return -1;
    }
    // static cast to a char pointer
    const unsigned char *bytes = static_cast<const unsigned char *>(buffer);
    for (ssize_t i = 0; i < result; i++) {
        // Log sent bytes
        io.sent.push_back(bytes[i]);
    }
    return result;
}
// Test the recv command which gets bytes from COSMOS (client)
ssize_t test_recv(int fd, void *buffer, size_t size, int flags) {
    // Think of command client as COSMOS sending a command
    EXPECT_EQ(fd, command_client);
    EXPECT_EQ(flags, 0);
    if (io.reads.empty()) {
        ADD_FAILURE() << "Unexpected recv: no planned reads remain";
        errno = EIO;
        return -1;
    }
    // Take the first item off the read queue to simulate reading data
    int result = io.reads.front();
    io.reads.pop_front();
    if (result < 0) {
        // Tac on a minus sign to make it an error code
        errno = static_cast<int>(-result);
        return -1;
    }
    if (static_cast<size_t>(result) > size || io.read_offset + result > io.command.size()) {
        ADD_FAILURE() << "Planned read exceeds the buffer or available command bytes";
        errno = EIO;
        return -1;
    }
    unsigned char *bytes = static_cast<unsigned char *>(buffer);
    for (int i = 0; i < result; i++) {
        bytes[i] = io.command[io.read_offset + i];
    }
    io.read_offset += result;
    return result;
}
} // extern "C"

namespace {
    TEST_F(TcpConnectionTest, AcceptsBothClientsAndPreservesExistingFlags) {
        int tlm, cmd, client_tlm, client_cmd;
        ASSERT_EQ(init_con(&tlm, &cmd, &client_tlm, &client_cmd, 8080, 8081), 0);

        EXPECT_EQ(tlm, telemetry_server);
        EXPECT_EQ(cmd, command_server);
        EXPECT_EQ(client_tlm, telemetry_client);
        EXPECT_EQ(client_cmd, command_client);
        const std::vector<std::pair<int, int>> expected = {
            {telemetry_client, O_APPEND | O_NONBLOCK},
            {command_client, O_APPEND | O_NONBLOCK}
        };
        EXPECT_EQ(io.nonblocking, expected);
        EXPECT_TRUE(io.closed.empty());
    }

    TEST_F(TcpConnectionTest, TelemetryEncodesNetworkBytesAndWrapsSequence) {
        // Test to ensure that our code makes sequence number in CCSDS packet wrap around
        seq_count = 0x3fff;
        ASSERT_EQ(send_telemetry(telemetry_client), 0);
        ASSERT_EQ(send_telemetry(telemetry_client), 0);

        // APID 1, unsegmented, length 3, temperature 42000 (0x0000a410).
        const std::vector<unsigned char> expected = {
            0x00, 0x01, 0xff, 0xff, 0x00, 0x03, 0x00, 0x00, 0xa4, 0x10,
            0x00, 0x01, 0xc0, 0x00, 0x00, 0x03, 0x00, 0x00, 0xa4, 0x10
        };
        EXPECT_EQ(io.sent, expected);
        EXPECT_EQ(io.temperature_reads, 2);
    }

    TEST_F(TcpConnectionTest, TelemetryRetriesInterruptedAndBusyPartialWrites) {
        // Test for various common connection errors and make sure code properly sends packet in these situations
        io.sends = {-EINTR, 3, -EAGAIN, 2, -EWOULDBLOCK, 5};
        ASSERT_EQ(send_telemetry(telemetry_client), 0);

        const std::vector<unsigned char> expected = {
            0x00, 0x01, 0xc0, 0x00, 0x00, 0x03, 0x00, 0x00, 0xa4, 0x10
        };
        EXPECT_EQ(io.sent, expected);
        EXPECT_TRUE(io.sends.empty());
        EXPECT_EQ(io.temperature_reads, 1);
    }

    TEST_F(TcpConnectionTest, TelemetryReportsMissingTemperatureFile) {
        io.fail_open = true;
        EXPECT_EQ(send_telemetry(telemetry_client), -1);
        EXPECT_TRUE(io.sent.empty());
        EXPECT_EQ(seq_count, 0);
    }

    TEST_F(TcpConnectionTest, TelemetryRejectsEmptyTemperature) {
        io.temperature = "";
        EXPECT_EQ(send_telemetry(telemetry_client), -1);
        EXPECT_TRUE(io.sent.empty());
        EXPECT_EQ(seq_count, 0);
    }

    TEST_F(TcpConnectionTest, TelemetryRejectsInvalidTemperature) {
        io.temperature = "invalid";
        EXPECT_EQ(send_telemetry(telemetry_client), -1);
        EXPECT_TRUE(io.sent.empty());
        EXPECT_EQ(seq_count, 0);
    }

    TEST_F(TcpConnectionTest, TelemetryStopsOnBrokenPipe) {
        io.sends = {-EPIPE};
        EXPECT_EQ(send_telemetry(telemetry_client), -1);
        EXPECT_TRUE(io.sent.empty());
        EXPECT_TRUE(io.sends.empty());
    }

    TEST_F(TcpConnectionTest, TelemetryStopsWhenPeerAcceptsNoBytes) {
        io.sends = {0};
        EXPECT_EQ(send_telemetry(telemetry_client), -1);
        EXPECT_TRUE(io.sent.empty());
        EXPECT_TRUE(io.sends.empty());
    }

    TEST_F(TcpConnectionTest, TelemetryStopsOnFailureAfterPartialWrite) {
        // Test how send telemetry works when connection is reset by peer
        io.sends = {3, -ECONNRESET};
        EXPECT_EQ(send_telemetry(telemetry_client), -1);
        EXPECT_EQ(io.sent.size(), 3);
        EXPECT_TRUE(io.sends.empty());
    }

    TEST_F(TcpConnectionTest, CommandWritesPayloadAndNullTerminator) {
        // Command packet would have 6 bytes for CCSDS header and then 12 for cmd string that is written to flag.txt
        command("sierra_space");
        io.reads = {19};
        ASSERT_EQ(receive_commands(command_client), 0);
        EXPECT_EQ(io.flag_contents, std::string("sierra_space\0", 13));
        EXPECT_TRUE(io.reads.empty());
    }

    TEST_F(TcpConnectionTest, CommandHandlesFragmentedHeaderAndPayload) {
        // Clark Griswold was here
        command("Clark");
        io.reads = {2, 5, 4};
        ASSERT_EQ(receive_commands(command_client), 0);
        EXPECT_EQ(io.flag_contents, std::string("Clark\0", 6));
        EXPECT_TRUE(io.reads.empty());
    }

    TEST_F(TcpConnectionTest, CommandLengthUsesBothNetworkOrderBytes) {
        command(std::string(258, 'x'));
        io.reads = {10, 254};
        ASSERT_EQ(receive_commands(command_client), 0);
        EXPECT_EQ(io.flag_contents, std::string(258, 'x') + '\0');
        EXPECT_TRUE(io.reads.empty());
    }

    TEST_F(TcpConnectionTest, CommandReportsFileCreationFailure) {
        command("hello");
        io.reads = {11};
        // Make fake file fail to open.. say that 5 times fast
        io.fail_open = true;
        EXPECT_EQ(receive_commands(command_client), -1);
        EXPECT_TRUE(io.flag_contents.empty());
    }

    TEST_F(TcpConnectionTest, CommandWaitsWhenReadWouldBlock) {
        io.reads = {-EWOULDBLOCK};
        EXPECT_EQ(receive_commands(command_client), 0);
        EXPECT_TRUE(io.flag_contents.empty());
    }

    TEST_F(TcpConnectionTest, CommandWaitsWhenReadShouldTryAgain) {
        io.reads = {-EAGAIN};
        EXPECT_EQ(receive_commands(command_client), 0);
        EXPECT_TRUE(io.flag_contents.empty());
    }

    TEST_F(TcpConnectionTest, CommandReportsReadFailure) {
        io.reads = {-ECONNRESET};
        EXPECT_EQ(receive_commands(command_client), -1);
        EXPECT_TRUE(io.flag_contents.empty());
    }

    TEST_F(TcpConnectionTest, CommandReportsDisconnection) {
        io.reads = {0};
        EXPECT_EQ(receive_commands(command_client), -1);
        EXPECT_TRUE(io.flag_contents.empty());
    }

    TEST_F(TcpConnectionTest, CommandReportsDisconnectionDuringHeader) {
        command("hello");
        io.reads = {2, 0};
        EXPECT_EQ(receive_commands(command_client), -1);
        EXPECT_TRUE(io.flag_contents.empty());
    }

    TEST_F(TcpConnectionTest, ServerClosesAllSocketsAfterTelemetryFailure) {
        io.fail_open = true;
        EXPECT_EQ(tcp_server_main(), 0);
        const std::vector<int> expected = {
            telemetry_client, command_client, telemetry_server, command_server
        };
        EXPECT_EQ(io.closed, expected);
        EXPECT_EQ(io.sleeps, 0);
    }

    TEST_F(TcpConnectionTest, ServerClosesAllSocketsAfterCommandFailure) {
        io.reads = {-EAGAIN, 0};
        EXPECT_EQ(tcp_server_main(), 0);
        const std::vector<int> expected = {
            telemetry_client, command_client, telemetry_server, command_server
        };
        EXPECT_EQ(io.closed, expected);
        EXPECT_EQ(io.sleeps, 1);
    }
}

/* Test-only substitutions; production is still compiled as C. */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif
int test_socket(int domain, int type, int protocol);
int test_setsockopt(int fd, int level, int option, const void *value, socklen_t size);
int test_bind(int fd, const struct sockaddr *address, socklen_t size);
int test_listen(int fd, int backlog);
int test_accept(int fd, struct sockaddr *address, socklen_t *size);
int test_fcntl(int fd, int command, ...);
int test_close(int fd);
unsigned int test_sleep(unsigned int seconds);
FILE *test_fopen(const char *path, const char *mode);
ssize_t test_send(int fd, const void *buffer, size_t size, int flags);
ssize_t test_recv(int fd, void *buffer, size_t size, int flags);
int tcp_server_main(void);
#ifdef __cplusplus
}
#endif

#ifdef TCP_CON_TEST_BUILD
#define socket test_socket
#define setsockopt test_setsockopt
#define bind test_bind
#define listen test_listen
#define accept test_accept
#define fcntl test_fcntl
#define close test_close
#define sleep test_sleep
#define fopen test_fopen
#define send test_send
#define recv test_recv
#define main tcp_server_main
#endif

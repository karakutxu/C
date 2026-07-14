#ifndef SOCKET_TEST_UTILS_H
#define SOCKET_TEST_UTILS_H

#include <stdint.h>

int create_server_socket(const char *path);

int connect_client_socket(const char *path);

int accept_client_socket(int server_fd);

void remove_socket(const char *path);

void cleanup_test_sockets(void);

void create_socket_directory(void);

#endif
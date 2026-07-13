#include "socket_test_utils.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include <string.h>

#include <stdio.h>

#include <stdlib.h>

#include <errno.h>

#include <sys/stat.h>

#define TEST_SOCKET_DIR "/tmp/prodrv_test"

void create_socket_directory(void)
{
    mkdir(TEST_SOCKET_DIR,0777);
}

void remove_socket(const char *path)
{
    unlink(path);
}

void cleanup_test_sockets(void)
{
    system("rm -f /tmp/prodrv_test/*.sock");
}

int create_server_socket(const char *path)
{
    int fd;

    struct sockaddr_un addr;

    fd=socket(AF_UNIX,SOCK_STREAM,0);

    if(fd<0)
        return -1;

    unlink(path);

    memset(&addr,0,sizeof(addr));

    addr.sun_family=AF_UNIX;

    strncpy(addr.sun_path,path,sizeof(addr.sun_path)-1);

    if(bind(fd,(struct sockaddr *)&addr,sizeof(addr)))
        return -1;

    if(listen(fd,5))
        return -1;

    return fd;
}

int connect_client_socket(const char *path)
{
    int fd;

    struct sockaddr_un addr;

    fd=socket(AF_UNIX,SOCK_STREAM,0);

    if(fd<0)
        return -1;

    memset(&addr,0,sizeof(addr));

    addr.sun_family=AF_UNIX;

    strncpy(addr.sun_path,path,sizeof(addr.sun_path)-1);

    if(connect(fd,(struct sockaddr *)&addr,sizeof(addr)))
        return -1;

    return fd;
}

int accept_client_socket(int server_fd)
{
    return accept(server_fd,NULL,NULL);
}
/*=====================================================================
 *
 * File: mock_csh_rx.c
 *
 *====================================================================*/

#include "mock_csh_rx.h"

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#include "socket_test_utils.h"
#include "test_component_common.h"

static int server_fd=-1;
static int client_fd=-1;
static int peer_fd=-1;

int mock_csh_rx_start(void)
{
    server_fd=create_server_socket(CSH_RX_SOCK_PATH);
    if(server_fd<0)
        return -1;

    client_fd=connect_client_socket(CSH_RX_SOCK_PATH);
    if(client_fd<0)
        return -1;

    peer_fd=accept_client_socket(server_fd);
    if(peer_fd<0)
        return -1;

    return 0;
}

void mock_csh_rx_stop(void)
{
    if(client_fd>=0) close(client_fd);
    if(peer_fd>=0) close(peer_fd);
    if(server_fd>=0) close(server_fd);

    client_fd=peer_fd=server_fd=-1;
}

int mock_csh_rx_send(const void *msg,uint32_t len)
{
    return send(peer_fd,msg,len,0);
}

int mock_csh_rx_fd(void)
{
    return peer_fd;
}
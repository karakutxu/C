/*=====================================================================
 *
 * File: test_component_common.c
 *
 *====================================================================*/

#include "test_component_common.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

ComponentFixture_t g_fixture;

static void reset_fixture(void)
{
    memset(&g_fixture,0,sizeof(g_fixture));

    g_fixture.ctrl.server_fd=-1;
    g_fixture.ctrl.client_fd=-1;
    g_fixture.ctrl.peer_fd=-1;

    g_fixture.slotmgr.server_fd=-1;
    g_fixture.slotmgr.client_fd=-1;
    g_fixture.slotmgr.peer_fd=-1;

    g_fixture.csh_evt.server_fd=-1;
    g_fixture.csh_evt.client_fd=-1;
    g_fixture.csh_evt.peer_fd=-1;

    g_fixture.csh_rx.server_fd=-1;
    g_fixture.csh_rx.client_fd=-1;
    g_fixture.csh_rx.peer_fd=-1;
}

void component_setup(void)
{
    reset_fixture();

    create_socket_directory();

    cleanup_test_sockets();

    component_create_servers();

    memset(&g_fixture.ctx,0,sizeof(g_fixture.ctx));

    init_context(0,&g_fixture.ctx);

    g_fixture.ifp=init_ifnet(&g_fixture.ctx);

    TEST_ASSERT_NOT_NULL(g_fixture.ifp);

    g_fixture.ps=init_protostatics(g_fixture.ifp,&g_fixture.ctx);

    TEST_ASSERT_NOT_NULL(g_fixture.ps);

    g_fixture.ifp->p=g_fixture.ps;

    g_fixture.pms=init_protomanstatics(g_fixture.ifp,
                                      &g_fixture.ctx);

    TEST_ASSERT_NOT_NULL(g_fixture.pms);
}

void component_teardown(void)
{
    if(g_fixture.ifp &&
       g_fixture.ps &&
       g_fixture.pms)
    {
        proto_shutdown(g_fixture.ifp,
                       &g_fixture.ctx,
                       g_fixture.ps,
                       g_fixture.pms);
    }

    component_close_all();

    cleanup_test_sockets();
}

void component_create_servers(void)
{
    g_fixture.ctrl.server_fd=
        create_server_socket(CTRL_SOCK_PATH);

    TEST_ASSERT_GREATER_OR_EQUAL(0,
                                 g_fixture.ctrl.server_fd);

    g_fixture.slotmgr.server_fd=
        create_server_socket(SLOTMGR_SOCK_PATH);

    TEST_ASSERT_GREATER_OR_EQUAL(0,
                                 g_fixture.slotmgr.server_fd);

    g_fixture.csh_evt.server_fd=
        create_server_socket(CSH_EVT_SOCK_PATH);

    TEST_ASSERT_GREATER_OR_EQUAL(0,
                                 g_fixture.csh_evt.server_fd);

    g_fixture.csh_rx.server_fd=
        create_server_socket(CSH_RX_SOCK_PATH);

    TEST_ASSERT_GREATER_OR_EQUAL(0,
                                 g_fixture.csh_rx.server_fd);
}

void component_accept_connections(void)
{
    g_fixture.ctrl.peer_fd=
        accept_client_socket(g_fixture.ctrl.server_fd);

    g_fixture.slotmgr.peer_fd=
        accept_client_socket(g_fixture.slotmgr.server_fd);

    g_fixture.csh_evt.peer_fd=
        accept_client_socket(g_fixture.csh_evt.server_fd);

    g_fixture.csh_rx.peer_fd=
        accept_client_socket(g_fixture.csh_rx.server_fd);
}

static void close_fd(int *fd)
{
    if(*fd>=0)
    {
        close(*fd);
        *fd=-1;
    }
}

void component_close_all(void)
{
    close_fd(&g_fixture.ctrl.server_fd);
    close_fd(&g_fixture.ctrl.client_fd);
    close_fd(&g_fixture.ctrl.peer_fd);

    close_fd(&g_fixture.slotmgr.server_fd);
    close_fd(&g_fixture.slotmgr.client_fd);
    close_fd(&g_fixture.slotmgr.peer_fd);

    close_fd(&g_fixture.csh_evt.server_fd);
    close_fd(&g_fixture.csh_evt.client_fd);
    close_fd(&g_fixture.csh_evt.peer_fd);

    close_fd(&g_fixture.csh_rx.server_fd);
    close_fd(&g_fixture.csh_rx.client_fd);
    close_fd(&g_fixture.csh_rx.peer_fd);
}

int component_send_ctrl(const void *msg,size_t len)
{
    return send(g_fixture.ctrl.peer_fd,msg,len,0);
}

int component_send_slotmgr(const void *msg,size_t len)
{
    return send(g_fixture.slotmgr.peer_fd,msg,len,0);
}

int component_send_csh_rx(const void *msg,size_t len)
{
    return send(g_fixture.csh_rx.peer_fd,msg,len,0);
}

int component_recv_slotmgr(void *msg,size_t len)
{
    return recv(g_fixture.slotmgr.peer_fd,msg,len,0);
}

int component_recv_csh_evt(void *msg,size_t len)
{
    return recv(g_fixture.csh_evt.peer_fd,msg,len,0);
}
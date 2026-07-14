/*=====================================================================
 *
 * File: test_component_common.h
 *
 *====================================================================*/

#ifndef TEST_COMPONENT_COMMON_H
#define TEST_COMPONENT_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <poll.h>

#include "unity.h"

#include "proto_main.h"
#include "message_factory.h"
#include "socket_test_utils.h"

/*--------------------------------------------------------------------
 * Socket paths
 *-------------------------------------------------------------------*/

#define TEST_SOCKET_DIR "/tmp/prodrv_test"

#define CTRL_SOCK_PATH      TEST_SOCKET_DIR "/ctrl.sock"
#define SLOTMGR_SOCK_PATH   TEST_SOCKET_DIR "/slotmgr.sock"
#define CSH_EVT_SOCK_PATH   TEST_SOCKET_DIR "/csh_evt.sock"
#define CSH_RX_SOCK_PATH    TEST_SOCKET_DIR "/csh_rx.sock"
#define PRODRV_SOCK_PATH    TEST_SOCKET_DIR "/prodrv.sock"

/*--------------------------------------------------------------------
 * Mock socket endpoints
 *-------------------------------------------------------------------*/

typedef struct
{
    int server_fd;
    int client_fd;
    int peer_fd;
} MockSocket_t;

/*--------------------------------------------------------------------
 * Harness context
 *-------------------------------------------------------------------*/

typedef struct
{
    MockSocket_t ctrl;
    MockSocket_t slotmgr;
    MockSocket_t csh_evt;
    MockSocket_t csh_rx;

    proto_context_t ctx;
    struct ifnet *ifp;
    struct proto_statics *ps;
    struct protoman_statics *pms;

} ComponentFixture_t;

/*--------------------------------------------------------------------
 * Global fixture
 *-------------------------------------------------------------------*/

extern ComponentFixture_t g_fixture;

/*--------------------------------------------------------------------
 * Setup / teardown
 *-------------------------------------------------------------------*/

void component_setup(void);
void component_teardown(void);

/*--------------------------------------------------------------------
 * Socket helpers
 *-------------------------------------------------------------------*/

void component_create_servers(void);

void component_accept_connections(void);

void component_close_all(void);

/*--------------------------------------------------------------------
 * Message helpers
 *-------------------------------------------------------------------*/

int component_send_ctrl(const void *msg,size_t len);

int component_send_slotmgr(const void *msg,size_t len);

int component_send_csh_rx(const void *msg,size_t len);

int component_recv_slotmgr(void *msg,size_t len);

int component_recv_csh_evt(void *msg,size_t len);

#endif
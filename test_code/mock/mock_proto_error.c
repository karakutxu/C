/*=====================================================================
 *
 * File: mock_proto_error.c
 *
 *====================================================================*/

#include "proto_main.h"

int mock_send_error_calls;

int mock_last_error;

void mock_proto_error_reset(void)
{
    mock_send_error_calls = 0;

    mock_last_error = 0;
}

void send_error_msg_pkt(struct ifnet *ifp,
                        int err,
                        proto_context_t *ctx)
{
    (void)ifp;
    (void)ctx;

    mock_send_error_calls++;

    mock_last_error = err;
}
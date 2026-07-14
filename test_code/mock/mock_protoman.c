/*=====================================================================
 *
 * File: mock_protoman.c
 *
 *====================================================================*/

#include "mock_protoman.h"

#include "protomanstatics_ng.h"
#include "proto_main.h"

int mock_protoman_calls;
MsgID_t mock_last_msgid;
int mock_protoman_return;

void mock_protoman_reset(void)
{
    mock_protoman_calls=0;
    mock_last_msgid=0;
    mock_protoman_return=0;
}

int protoman_socket(struct protoman_statics *pms,
                    void *sock,
                    MsgID_t msg_id,
                    char *args,
                    proto_context_t *ctx)
{
    (void)pms;
    (void)sock;
    (void)args;
    (void)ctx;

    mock_protoman_calls++;

    mock_last_msgid=msg_id;

    return mock_protoman_return;
}
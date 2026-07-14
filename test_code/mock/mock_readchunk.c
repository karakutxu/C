/*=====================================================================
 *
 * File: mock_readchunk.c
 *
 *====================================================================*/

#include "mock_readchunk.h"

#include "proto_main.h"

int mock_readchunk_calls;

void mock_readchunk_reset(void)
{
    mock_readchunk_calls=0;
}

int read_CSHchunk_ng(struct ifnet *ifp,
                     struct RX_LAYER_2_CHUNK *chunk,
                     proto_context_t *ctx)
{
    (void)ifp;
    (void)chunk;
    (void)ctx;

    mock_readchunk_calls++;

    return 0;
}
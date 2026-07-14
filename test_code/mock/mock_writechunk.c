/*=====================================================================
 *
 * File: mock_writechunk.c
 *
 *====================================================================*/

#include "mock_writechunk.h"

#include "ifnet_compat.h"

int mock_writechunk_calls;

int mock_writechunk_return;

int mock_last_writechunk_chunks;

int mock_last_writechunk_frame_len;

void mock_writechunk_reset(void)
{
    mock_writechunk_calls = 0;

    mock_writechunk_return = 0;

    mock_last_writechunk_chunks = 0;

    mock_last_writechunk_frame_len = 0;
}

int write_chunk2CSH(struct ifnet *ifp,
                    struct TX_LAYER_2_CHUNK *chunk,
                    uint16_t n_chunks,
                    uint16_t frame_len)
{
    (void)ifp;
    (void)chunk;

    mock_writechunk_calls++;

    mock_last_writechunk_chunks = n_chunks;

    mock_last_writechunk_frame_len = frame_len;

    return mock_writechunk_return;
}
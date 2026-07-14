/*=====================================================================
 *
 * File: mock_makeframe.c
 *
 *====================================================================*/

#include "mock_makeframe.h"

#include "makeframe_ng.h"

int mock_makeframe_calls;
int mock_makeframe_return;
uint16_t mock_frame_length;

void mock_makeframe_reset(void)
{
    mock_makeframe_calls=0;
    mock_makeframe_return=0;
    mock_frame_length=280;
}

int make_L2frame_ng(struct ifnet *ifp,
                    struct TX_L2_FRAME_PARAMS *params,
                    uint16_t *frame_len)
{
    (void)ifp;
    (void)params;

    mock_makeframe_calls++;

    *frame_len=mock_frame_length;

    return mock_makeframe_return;
}

int write_chunk2CSH(struct ifnet *ifp,
                    void *chunk,
                    uint16_t n,
                    uint16_t frame_len)
{
    (void)ifp;
    (void)chunk;
    (void)n;
    (void)frame_len;

    return 0;
}
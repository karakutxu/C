#include "mock_makeframe.h"
#include <stddef.h>

int mock_makeframe_calls = 0;
int mock_makeframe_return = 0;
uint16_t mock_frame_length = 0;

int mock_writechunk_calls = 0;
int mock_last_writechunk_frame_len = 0;
int mock_last_writechunk_chunks = 0;
int mock_writechunk_return = 0;

void mock_makeframe_reset(void)
{
    mock_makeframe_calls = 0;
    mock_makeframe_return = 0;
    mock_frame_length = 0;

    mock_writechunk_calls = 0;
    mock_last_writechunk_frame_len = 0;
    mock_last_writechunk_chunks = 0;
    mock_writechunk_return = 0;
}

int make_L2frame_ng(struct ifnet *ifp, struct TX_L2_FRAME_PARAMS *params, uint16_t *ret_len)
{
    (void)ifp;
    (void)params;

    mock_makeframe_calls++;
    if (ret_len != NULL)
    {
        *ret_len = mock_frame_length;
    }
    return mock_makeframe_return;
}

int write_chunk2CSH(struct ifnet *ifp, struct TX_LAYER_2_CHUNK *p, uint16_t clen, uint16_t flen)
{
    (void)ifp;
    (void)p;

    mock_writechunk_calls++;
    mock_last_writechunk_chunks = (int)clen;
    mock_last_writechunk_frame_len = (int)flen;
    return mock_writechunk_return;
}
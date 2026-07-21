#include "mock_writechunk.h"

void mock_writechunk_reset(void)
{
    mock_writechunk_calls = 0;
    mock_writechunk_return = 0;
    mock_last_writechunk_chunks = 0;
    mock_last_writechunk_frame_len = 0;
}
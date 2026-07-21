#ifndef MOCK_MAKEFRAME_H
#define MOCK_MAKEFRAME_H

#include <stdint.h>
#include "ifnet.h"
#include "dpr1map.h"

extern int mock_makeframe_calls;
extern int mock_makeframe_return;
extern uint16_t mock_frame_length;

extern int mock_writechunk_calls;
extern int mock_last_writechunk_frame_len;
extern int mock_last_writechunk_chunks;
extern int mock_writechunk_return;

void mock_makeframe_reset(void);

#endif /* MOCK_MAKEFRAME_H */
#ifndef MOCK_WRITECHUNK_H
#define MOCK_WRITECHUNK_H

#include <stdint.h>

extern int mock_writechunk_calls;
extern int mock_writechunk_return;
extern int mock_last_writechunk_chunks;
extern int mock_last_writechunk_frame_len;

void mock_writechunk_reset(void);

#endif /* MOCK_WRITECHUNK_H */
/*=====================================================================
 *
 * File: mock_makeframe.h
 *
 *====================================================================*/

#ifndef MOCK_MAKEFRAME_H
#define MOCK_MAKEFRAME_H

#include <stdint.h>

struct ifnet;
struct TX_L2_FRAME_PARAMS;

extern int mock_makeframe_calls;
extern int mock_makeframe_return;

extern uint16_t mock_frame_length;

void mock_makeframe_reset(void);

#endif
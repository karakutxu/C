/*=====================================================================
 *
 * File: mock_protoman.h
 *
 *====================================================================*/

#ifndef MOCK_PROTOMAN_H
#define MOCK_PROTOMAN_H

#include "hcs_messages.h"

extern int mock_protoman_calls;
extern MsgID_t mock_last_msgid;
extern int mock_protoman_return;

void mock_protoman_reset(void);

#endif
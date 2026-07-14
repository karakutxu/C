/*=====================================================================
 *
 * File: mock_slotmgr.h
 *
 *====================================================================*/

#ifndef MOCK_SLOTMGR_H
#define MOCK_SLOTMGR_H

#include <stdint.h>

int mock_slotmgr_start(void);
void mock_slotmgr_stop(void);

int mock_slotmgr_send(const void *msg,uint32_t len);

int mock_slotmgr_receive(void *msg,uint32_t len);

int mock_slotmgr_fd(void);

#endif
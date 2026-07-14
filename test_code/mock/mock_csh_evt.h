/*=====================================================================
 *
 * File: mock_csh_evt.h
 *
 *====================================================================*/

#ifndef MOCK_CSH_EVT_H
#define MOCK_CSH_EVT_H

#include <stdint.h>

int mock_csh_evt_start(void);
void mock_csh_evt_stop(void);

int mock_csh_evt_receive(void *msg,uint32_t len);

int mock_csh_evt_fd(void);

#endif
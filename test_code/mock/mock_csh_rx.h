/*=====================================================================
 *
 * File: mock_csh_rx.h
 *
 *====================================================================*/

#ifndef MOCK_CSH_RX_H
#define MOCK_CSH_RX_H

#include <stdint.h>

int mock_csh_rx_start(void);
void mock_csh_rx_stop(void);

int mock_csh_rx_send(const void *msg,uint32_t len);

int mock_csh_rx_fd(void);

#endif
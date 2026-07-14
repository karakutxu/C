/*=====================================================================
 *
 * File: mock_ctrl.h
 *
 *====================================================================*/

#ifndef MOCK_CTRL_H
#define MOCK_CTRL_H

#include <stdint.h>

int mock_ctrl_start(void);
void mock_ctrl_stop(void);

int mock_ctrl_send(const void *msg, uint32_t len);

int mock_ctrl_receive(void *msg, uint32_t len);

int mock_ctrl_fd(void);

#endif
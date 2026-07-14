/*=====================================================================
 *
 * File: mock_logger.h
 *
 *====================================================================*/

#ifndef MOCK_LOGGER_H
#define MOCK_LOGGER_H

extern int mock_log_count;
extern int mock_last_level;

void mock_logger_reset(void);

#endif
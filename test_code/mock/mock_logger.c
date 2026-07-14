/*=====================================================================
 *
 * File: mock_logger.c
 *
 *====================================================================*/

#include "mock_logger.h"

#include <stdarg.h>

int mock_log_count;
int mock_last_level;

void mock_logger_reset(void)
{
    mock_log_count=0;
    mock_last_level=0;
}

void LOG(int level,const char *fmt,...)
{
    (void)fmt;

    mock_log_count++;

    mock_last_level=level;
}

void LOG_INFO(const char *fmt,...)
{
    (void)fmt;
}

void LOG_WARNING(const char *fmt,...)
{
    (void)fmt;
}

void LOG_ERR(const char *fmt,...)
{
    (void)fmt;
}
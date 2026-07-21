#include "mock_logger.h"
#include <stdarg.h>

int mock_logger_calls = 0;
int mock_logger_last_level = 0;

void mock_logger_reset(void)
{
    mock_logger_calls = 0;
    mock_logger_last_level = 0;
}

void LOG(int level, const char *fmt, ...)
{
    mock_logger_calls++;
    mock_logger_last_level = level;
    (void)fmt;
}
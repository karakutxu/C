#ifndef MOCK_LOGGER_H
#define MOCK_LOGGER_H

#include "debug_logger.h"

extern int mock_logger_calls;
extern int mock_logger_last_level;

void mock_logger_reset(void);

#endif /* MOCK_LOGGER_H */
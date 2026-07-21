#include "time_compat.h"

uint32_t nanotime(uint32_t *secs)
{
    if (secs != NULL)
    {
        *secs = 1000;
    }
    return 500000;
}
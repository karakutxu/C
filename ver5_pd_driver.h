#ifndef PRODRV_DRIVER_H
#define PRODRV_DRIVER_H

#include <stdint.h>

#include "msg_handler.h"

typedef struct
{
    int (*build_frame)(
        ScheduleMeta_t *meta,
        const uint8_t *payload,
        uint32_t payload_len);

    int (*send_frame)(void);

} ProdrvTxOps_t;

typedef struct
{
    int (*read_frame)(void);
    int (*decode_frame)(
        uint8_t *out,
        uint32_t *out_len);

} ProdrvRxOps_t;

extern ProdrvTxOps_t g_prodrv_tx_ops;
extern ProdrvRxOps_t g_prodrv_rx_ops;

#endif
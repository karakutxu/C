#include "prodrv_driver.h"

static int build_frame_impl(
        ScheduleMeta_t *meta,
        const uint8_t *payload,
        uint32_t payload_len)
{
    /*
     * TODO:
     * call make_L2frame()
     */
    return 0;
}

static int send_frame_impl(void)
{
    /*
     * TODO:
     * call write_chunk2CSH()
     */
    return 0;
}

static int read_frame_impl(void)
{
    /*
     * TODO:
     * call read_VMEchunk()
     */
    return 0;
}

static int decode_frame_impl(
        uint8_t *out,
        uint32_t *out_len)
{
    /*
     * TODO
     */
    return 0;
}

ProdrvTxOps_t g_prodrv_tx_ops =
{
    .build_frame = build_frame_impl,
    .send_frame  = send_frame_impl,
};

ProdrvRxOps_t g_prodrv_rx_ops =
{
    .read_frame   = read_frame_impl,
    .decode_frame = decode_frame_impl,
};
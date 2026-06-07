#include <string.h>
#include <sys/socket.h>

#include "logger.h"
#include "prodrv_driver.h"
#include "prodrv_handlers.h"

void handle_msg_init_prodrv(
        ProdrvContext_t *ctx,
        uint8_t *buf)
{
    MsgInitProdrv_t *msg =
        (MsgInitProdrv_t *)buf;

    ctx->config.chunk1_bytes =
        msg->chunk1_bytes;

    ctx->config.prepare_12_Frame_to_tx_ms =
        msg->prepare_l2_frame_to_tx_ms;

    ctx->init_received = true;

    MsgAck_t ack =
    {
        .id = MSG_ACK,
        .hcs_index = ctx->hcs_index,
        .ack_msg = MSG_INIT_PRODRV
    };

    send(
        ctx->slotmgr_fd,
        &ack,
        sizeof(ack),
        0);

    HALO_NG_LOG(
        INFO,
        LOC,
        format("[%s] MSG_INIT_PRODRV",
        ctx->proc_name));
}

void handle_msg_comm_param(
        ProdrvContext_t *ctx,
        uint8_t *buf)
{
    memcpy(
        &ctx->comm_param,
        buf,
        sizeof(MsgCommParam_t));

    ctx->comm_param_received = true;

    HALO_NG_LOG(
        INFO,
        LOC,
        format("[%s] MSG_COMM_PARAM",
        ctx->proc_name));
}

void handle_msg_prepare_frame(
        ProdrvContext_t *ctx,
        uint8_t *buf)
{
    MsgPrepareFrame_t *msg =
        (MsgPrepareFrame_t *)buf;

    g_prodrv_tx_ops.build_frame(
        &msg->sch_meta,
        NULL,
        0);

    g_prodrv_tx_ops.send_frame();

    MsgTxStart_t tx_start;

    memset(&tx_start, 0, sizeof(tx_start));

    tx_start.id = MSG_TX_START;
    tx_start.tr_id = msg->tr_id;

    memcpy(
        &tx_start.sch_meta,
        &msg->sch_meta,
        sizeof(ScheduleMeta_t));

    send(
        ctx->csh_evt_fd,
        &tx_start,
        sizeof(tx_start),
        0);
}

void handle_msg_tx_stats(
        ProdrvContext_t *ctx,
        uint8_t *buf)
{
    MsgTxStats_t *msg =
        (MsgTxStats_t *)buf;

    MsgTxProcStats_t stats;

    stats.id = MSG_TX_PROC_STATS;
    stats.tr_id = msg->tr_id;
    stats.actual_tx_lrtc =
        msg->actual_tx_lrtc;

    send(
        ctx->pme_fd,
        &stats,
        sizeof(stats),
        0);
}

void handle_msg_rx_stats(
        ProdrvContext_t *ctx,
        uint8_t *buf)
{
    MsgRxStats_t *msg =
        (MsgRxStats_t *)buf;

    MsgRxProcStats_t stats;

    stats.id = MSG_RX_PROC_STATS;
    stats.tr_id = msg->tr_id;

    stats.actual_rx_lrtc =
        msg->actual_rx_lrtc;

    stats.cmp_time =
        msg->cmp_time;

    stats.n_block_err =
        msg->n_block_err;

    send(
        ctx->pme_fd,
        &stats,
        sizeof(stats),
        0);
}

void handle_msg_demod_data(
        ProdrvContext_t *ctx,
        uint8_t *buf)
{
    MsgDemodData_t *msg =
        (MsgDemodData_t *)buf;

    HALO_NG_LOG(
        INFO,
        LOC,
        format("[%s] MSG_DEMOD_DATA tr=0x%04x",
        ctx->proc_name,
        msg->tr_id));
}
#include "message_factory.h"

static uint16_t g_transaction = 1;

static ScheduleMeta_t default_schedule(void)
{
    ScheduleMeta_t s;

    memset(&s,0,sizeof(s));

    s.lrtc           = 123456;

    s.time.sec       = 100;

    s.time.usec      = 500000;

    s.comm_type      = AIR_INTERFACE;

    s.ll_type        = LL_OPEN;

    s.payload_type   = RAW;

    s.tx_rx_type     = TX;

    s.radio_freq     = 30000000;

    s.radio_power    = MEDIUM;

    return s;
}

void mf_reset(void)
{
    g_transaction = 1;
}

uint16_t mf_next_transaction(void)
{
    return g_transaction++;
}

/********************************************************************
 *
 * INIT
 *
 ********************************************************************/

MsgInitProdrv_t mf_init_prodrv(void)
{
    MsgInitProdrv_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_INIT_PRODRV;

    m.chunk1_bytes = 1024;

    m.prepare_l2_frame_to_tx_ms = 5;

    return m;
}

MsgInitSlotmgr_t mf_init_slotmgr(void)
{
    MsgInitSlotmgr_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_INIT_SLOTMGR;

    return m;
}

MsgInitCsh_t mf_init_csh(void)
{
    MsgInitCsh_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_INIT_CSH;

    m.data_rate_bps = 9600;

    m.op_mode = NORMAL;

    return m;
}

MsgCommParam_t mf_comm_param(void)
{
    MsgCommParam_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_COMM_PARAM;

    m.data_rate_bps = 9600;

    m.op_mode = NORMAL;

    return m;
}

/********************************************************************
 *
 * Scheduling
 *
 ********************************************************************/

MsgPrepareFrame_t mf_prepare_frame(void)
{
    MsgPrepareFrame_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_PREPARE_FRAME;

    m.tr_id = mf_next_transaction();

    m.sch_meta = default_schedule();

    return m;
}

MsgRxStart_t mf_rx_start(void)
{
    MsgRxStart_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_RX_START;

    m.tr_id = mf_next_transaction();

    m.sch_rx_lrtc = 987654;

    m.comm_type = AIR_INTERFACE;

    m.ll_type = LL_OPEN;

    m.tx_rx_type = RX;

    m.op_mode = NORMAL;

    m.radio_freq = 30000000;

    return m;
}

/********************************************************************
 *
 * TX
 *
 ********************************************************************/

MsgTxStart_t mf_tx_start(void)
{
    MsgTxStart_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_TX_START;

    m.tr_id = mf_next_transaction();

    m.sch_meta = default_schedule();

    memset(m.tx_payload,
           0x55,
           sizeof(m.tx_payload));

    return m;
}

MsgProcTx_t mf_proc_tx(void)
{
    MsgProcTx_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_PROC_TX;

    m.tr_id = mf_next_transaction();

    m.sch_meta = default_schedule();

    memset(m.tx_payload,
           0xaa,
           sizeof(m.tx_payload));

    return m;
}

MsgTxPkt_t mf_tx_pkt(void)
{
    MsgTxPkt_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_TX_PKT;

    m.tr_id = mf_next_transaction();

    m.sch_lrtc = 99999;

    m.op_mode = NORMAL;

    return m;
}

MsgTxStatus_t mf_tx_status(void)
{
    MsgTxStatus_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_TX_STATUS;

    m.tr_id = mf_next_transaction();

    m.status = TX_SUCCESS;

    m.actual_tx_lrtc = 100001;

    return m;
}

MsgTxStats_t mf_tx_stats(void)
{
    MsgTxStats_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_TX_STATS;

    m.tr_id = mf_next_transaction();

    m.status = TX_SUCCESS;

    m.actual_tx_lrtc = 100001;

    return m;
}

/********************************************************************
 *
 * RX
 *
 ********************************************************************/

MsgRxStatus_t mf_rx_status(void)
{
    MsgRxStatus_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_RX_STATUS;

    m.tr_id = mf_next_transaction();

    m.status = RX_SUCCESS;

    m.actual_rx_lrtc = 200001;

    memset(m.rx_pkt,
           0x33,
           sizeof(m.rx_pkt));

    return m;
}

MsgRxStats_t mf_rx_stats(void)
{
    MsgRxStats_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_RX_STATS;

    m.tr_id = mf_next_transaction();

    m.status = RX_SUCCESS;

    m.actual_rx_lrtc = 200001;

    m.cmp_time.sec = 1000;

    m.cmp_time.usec = 123456;

    m.n_block_err = 0;

    return m;
}

MsgDemodData_t mf_demod_data(void)
{
    MsgDemodData_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_DEMOD_DATA;

    m.tr_id = mf_next_transaction();

    memset(m.demod_bytes,
           0x99,
           sizeof(m.demod_bytes));

    return m;
}

/********************************************************************
 *
 * ACK
 *
 ********************************************************************/

MsgAck_t mf_ack(MsgID_t ack_for)
{
    MsgAck_t m;

    memset(&m,0,sizeof(m));

    m.id = MSG_ACK;

    m.hcs_index = 0;

    m.ack_msg = ack_for;

    return m;
}
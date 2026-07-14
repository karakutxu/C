#ifndef MESSAGE_FACTORY_H
#define MESSAGE_FACTORY_H

#include <stdint.h>
#include <string.h>

#include "msg_handler.h"

/*
 * Message Factory
 *
 * Produces valid default messages that can be modified by
 * individual tests.
 */

#ifdef __cplusplus
extern "C" {
#endif

void mf_reset(void);

uint16_t mf_next_transaction(void);

/*--------------------------------------------------------------------
 * Initialisation messages
 *-------------------------------------------------------------------*/

MsgInitProdrv_t      mf_init_prodrv(void);
MsgInitSlotmgr_t     mf_init_slotmgr(void);
MsgInitCsh_t         mf_init_csh(void);
MsgCommParam_t       mf_comm_param(void);

/*--------------------------------------------------------------------
 * Scheduling
 *-------------------------------------------------------------------*/

MsgPrepareFrame_t    mf_prepare_frame(void);
MsgRxStart_t         mf_rx_start(void);

/*--------------------------------------------------------------------
 * TX path
 *-------------------------------------------------------------------*/

MsgTxStart_t         mf_tx_start(void);
MsgProcTx_t          mf_proc_tx(void);
MsgTxPkt_t           mf_tx_pkt(void);
MsgTxStatus_t        mf_tx_status(void);
MsgTxStats_t         mf_tx_stats(void);

/*--------------------------------------------------------------------
 * RX path
 *-------------------------------------------------------------------*/

MsgRxStatus_t        mf_rx_status(void);
MsgRxStats_t         mf_rx_stats(void);
MsgDemodData_t       mf_demod_data(void);

/*--------------------------------------------------------------------
 * ACK
 *-------------------------------------------------------------------*/

MsgAck_t             mf_ack(MsgID_t ack_for);

#ifdef __cplusplus
}
#endif

#endif
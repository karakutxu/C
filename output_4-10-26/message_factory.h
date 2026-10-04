#ifndef MESSAGE_FACTORY_H
#define MESSAGE_FACTORY_H

#include <stdint.h>
#include <string.h>

#include "msg_handler.h"

/*
 * Existing factory functions
 */
void mf_reset(void);
uint16_t mf_next_transaction(void);

MsgInitProdrv_t mf_init_prodrv(void);
MsgInitSlotmgr_t mf_init_slotmgr(void);
MsgInitCsh_t mf_init_csh(void);
MsgCommParam_t mf_comm_param(void);

MsgPrepareFrame_t mf_prepare_frame(void);
MsgRxStart_t mf_rx_start(void);

MsgTxStart_t mf_tx_start(void);
MsgProcTx_t mf_proc_tx(void);
MsgTxPkt_t mf_tx_pkt(void);
MsgTxStatus_t mf_tx_status(void);
MsgTxStats_t mf_tx_stats(void);

MsgRxStatus_t mf_rx_status(void);
MsgRxStats_t mf_rx_stats(void);
MsgDemodData_t mf_demod_data(void);

MsgAck_t mf_ack(MsgID_t ack_for);


/*
 * ------------------------------------------------------------------
 * PROTOMAN control messages
 * ------------------------------------------------------------------
 *
 * The wire format is:
 *
 *     +----------------+----------------------+
 *     | MsgID_t        | command argument     |
 *     +----------------+----------------------+
 *
 * handle_MSG_PROTOMAN_CONTROL() passes the address immediately
 * after MsgID_t to protoman_socket().
 */

/* 1. MSG_PROTO_RESET */
typedef struct
{
    MsgID_t id;
} MsgResetProdrv_t;


/* 2. MSG_SET_PROTO_DRV_DBG_LEVEL */
typedef struct
{
    MsgID_t id;
    int_4 dbg_level;
} MsgSetProtoDrvDbgLevel_t;


/* 3. MSG_INIT_PREPARE_FRAME_STATS */
typedef struct
{
    MsgID_t id;
} MsgInitPrepareFrameStats_t;


/*
 * 4. MSG_GET_PREPARE_FRAME_STATS
 *
 * struct pf_stats is the existing production structure.
 */
typedef struct
{
    MsgID_t id;
    struct pf_stats stats;
} MsgGetPrepareFrameStats_t;


/* 5. MSG_SET_PROTO_DRV_MTU */
typedef struct
{
    MsgID_t id;
    u_long mtu;
} MsgSetProtoDrvMtu_t;


/* 6. MSG_SET_LOCAL_ADDRESS */
typedef struct
{
    MsgID_t id;
    L2_addr_t addr;
} MsgSetLocalAddress_t;


/* 7. MSG_INITIALISE_COMPRESSION_DICTIONARY */
typedef struct
{
    MsgID_t id;
    L2_addr_t addr;
} MsgInitialiseCompressionDictionary_t;


/* 8. MSG_INITIALISE_TX_COMPRESSION_DICTIONARY */
typedef struct
{
    MsgID_t id;
    L2_addr_t addr;
} MsgInitialiseTxCompressionDictionary_t;


/* 9. MSG_INITIALISE_REPLAY_COUNTERS */
typedef struct
{
    MsgID_t id;
    L2_addr_t addr;
} MsgInitialiseReplayCounters_t;


/* 10. MSG_ENABLE_LINK */
typedef struct
{
    MsgID_t id;
    L2_addr_t addr;
} MsgEnableLink_t;


/* 11. MSG_DISABLE_LINK */
typedef struct
{
    MsgID_t id;
    L2_addr_t addr;
} MsgDisableLink_t;


/* 12. MSG_FLUSH */
typedef struct
{
    MsgID_t id;
    L2_addr_t addr;
} MsgFlush_t;


/* 13. MSG_DELETE_LINK */
typedef struct
{
    MsgID_t id;
    L2_addr_t addr;
} MsgDeleteLink_t;


/* 14. MSG_SET_CLP_PRIORITY */
typedef struct
{
    MsgID_t id;
    struct set_clp_priority_t data;
} MsgSetClpPriority_t;


/* 15. MSG_GET_CLP_PRIORITY */
typedef struct
{
    MsgID_t id;
    struct get_clp_priority_t data;
} MsgGetClpPriority_t;


/* 16. MSG_GET_CLP_PRIORITIES */
typedef struct
{
    MsgID_t id;
    priority_t priorities[N_L2_ADDRS];
} MsgGetClpPriorities_t;


/* 17. MSG_PUT_CLP_PRIORITIES */
typedef struct
{
    MsgID_t id;
    priority_t priorities[N_L2_ADDRS];
} MsgPutClpPriorities_t;


/* 18. MSG_DISABLE_TCP_FRAGMENTATION */
typedef struct
{
    MsgID_t id;
} MsgDisableTcpFragmentation_t;


/*
 * 19. MSG_INITIALISE_FLOW
 *
 * The production switch expects:
 *
 *     struct initialise_flow_t *
 *
 * so we reuse that production structure rather than inventing
 * another definition.
 */
typedef struct
{
    MsgID_t id;
    struct initialise_flow_t data;
} MsgInitialiseFlow_t;


/*
 * 20. MSG_RETRIEVE_BUFFER_UTILISATION
 */
typedef struct
{
    MsgID_t id;
    struct ret_buffer_util_t data;
} MsgRetrieveBufferUtilisation_t;


/*
 * Factory functions
 */

MsgResetProdrv_t
mf_reset_prodrv(void);

MsgSetProtoDrvDbgLevel_t
mf_set_proto_drv_dbg_level(int_4 level);

MsgInitPrepareFrameStats_t
mf_init_prepare_frame_stats(void);

MsgGetPrepareFrameStats_t
mf_get_prepare_frame_stats(void);

MsgSetProtoDrvMtu_t
mf_set_proto_drv_mtu(u_long mtu);

MsgSetLocalAddress_t
mf_set_local_address(L2_addr_t addr);

MsgInitialiseCompressionDictionary_t
mf_initialise_compression_dictionary(L2_addr_t addr);

MsgInitialiseTxCompressionDictionary_t
mf_initialise_tx_compression_dictionary(L2_addr_t addr);

MsgInitialiseReplayCounters_t
mf_initialise_replay_counters(L2_addr_t addr);

MsgEnableLink_t
mf_enable_link(L2_addr_t addr);

MsgDisableLink_t
mf_disable_link(L2_addr_t addr);

MsgFlush_t
mf_flush(L2_addr_t addr);

MsgDeleteLink_t
mf_delete_link(L2_addr_t addr);

MsgSetClpPriority_t
mf_set_clp_priority(L2_addr_t addr, priority_t priority);

MsgGetClpPriority_t
mf_get_clp_priority(L2_addr_t addr);

MsgGetClpPriorities_t
mf_get_clp_priorities(void);

MsgPutClpPriorities_t
mf_put_clp_priorities(priority_t default_priority);

MsgDisableTcpFragmentation_t
mf_disable_tcp_fragmentation(void);

MsgInitialiseFlow_t
mf_initialise_flow(void);

MsgRetrieveBufferUtilisation_t
mf_retrieve_buffer_utilisation(void);

#endif
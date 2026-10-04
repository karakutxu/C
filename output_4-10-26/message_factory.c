#include "message_factory.h"

/*
 * Existing g_transaction, mf_reset(), mf_next_transaction(),
 * and existing factory functions remain unchanged.
 */


/********************************************************************
 *
 * PROTOMAN CONTROL
 *
 ********************************************************************/

MsgResetProdrv_t mf_reset_prodrv(void)
{
    MsgResetProdrv_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_PROTO_RESET;

    return m;
}


/*
 * MSG_SET_PROTO_DRV_DBG_LEVEL
 */
MsgSetProtoDrvDbgLevel_t
mf_set_proto_drv_dbg_level(int_4 level)
{
    MsgSetProtoDrvDbgLevel_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_SET_PROTO_DRV_DBG_LEVEL;
    m.dbg_level = level;

    return m;
}


/*
 * MSG_INIT_PREPARE_FRAME_STATS
 */
MsgInitPrepareFrameStats_t
mf_init_prepare_frame_stats(void)
{
    MsgInitPrepareFrameStats_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_INIT_PREPARE_FRAME_STATS;

    return m;
}


/*
 * MSG_GET_PREPARE_FRAME_STATS
 */
MsgGetPrepareFrameStats_t
mf_get_prepare_frame_stats(void)
{
    MsgGetPrepareFrameStats_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_GET_PREPARE_FRAME_STATS;

    return m;
}


/*
 * MSG_SET_PROTO_DRV_MTU
 */
MsgSetProtoDrvMtu_t
mf_set_proto_drv_mtu(u_long mtu)
{
    MsgSetProtoDrvMtu_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_SET_PROTO_DRV_MTU;
    m.mtu = mtu;

    return m;
}


/*
 * MSG_SET_LOCAL_ADDRESS
 */
MsgSetLocalAddress_t
mf_set_local_address(L2_addr_t addr)
{
    MsgSetLocalAddress_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_SET_LOCAL_ADDRESS;
    m.addr = addr;

    return m;
}


/*
 * MSG_INITIALISE_COMPRESSION_DICTIONARY
 */
MsgInitialiseCompressionDictionary_t
mf_initialise_compression_dictionary(L2_addr_t addr)
{
    MsgInitialiseCompressionDictionary_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_INITIALISE_COMPRESSION_DICTIONARY;
    m.addr = addr;

    return m;
}


/*
 * MSG_INITIALISE_TX_COMPRESSION_DICTIONARY
 */
MsgInitialiseTxCompressionDictionary_t
mf_initialise_tx_compression_dictionary(L2_addr_t addr)
{
    MsgInitialiseTxCompressionDictionary_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_INITIALISE_TX_COMPRESSION_DICTIONARY;
    m.addr = addr;

    return m;
}


/*
 * MSG_INITIALISE_REPLAY_COUNTERS
 */
MsgInitialiseReplayCounters_t
mf_initialise_replay_counters(L2_addr_t addr)
{
    MsgInitialiseReplayCounters_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_INITIALISE_REPLAY_COUNTERS;
    m.addr = addr;

    return m;
}


/*
 * MSG_ENABLE_LINK
 */
MsgEnableLink_t
mf_enable_link(L2_addr_t addr)
{
    MsgEnableLink_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_ENABLE_LINK;
    m.addr = addr;

    return m;
}


/*
 * MSG_DISABLE_LINK
 */
MsgDisableLink_t
mf_disable_link(L2_addr_t addr)
{
    MsgDisableLink_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_DISABLE_LINK;
    m.addr = addr;

    return m;
}


/*
 * MSG_FLUSH
 */
MsgFlush_t
mf_flush(L2_addr_t addr)
{
    MsgFlush_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_FLUSH;
    m.addr = addr;

    return m;
}


/*
 * MSG_DELETE_LINK
 */
MsgDeleteLink_t
mf_delete_link(L2_addr_t addr)
{
    MsgDeleteLink_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_DELETE_LINK;
    m.addr = addr;

    return m;
}


/*
 * MSG_SET_CLP_PRIORITY
 */
MsgSetClpPriority_t
mf_set_clp_priority(L2_addr_t addr, priority_t priority)
{
    MsgSetClpPriority_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_SET_CLP_PRIORITY;

    m.data.addr = addr;
    m.data.pri = priority;

    return m;
}


/*
 * MSG_GET_CLP_PRIORITY
 */
MsgGetClpPriority_t
mf_get_clp_priority(L2_addr_t addr)
{
    MsgGetClpPriority_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_GET_CLP_PRIORITY;

    m.data.addr = addr;

    return m;
}


/*
 * MSG_GET_CLP_PRIORITIES
 */
MsgGetClpPriorities_t
mf_get_clp_priorities(void)
{
    MsgGetClpPriorities_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_GET_CLP_PRIORITIES;

    return m;
}


/*
 * MSG_PUT_CLP_PRIORITIES
 */
MsgPutClpPriorities_t
mf_put_clp_priorities(priority_t default_priority)
{
    MsgPutClpPriorities_t m;
    int i;

    memset(&m, 0, sizeof(m));

    m.id = MSG_PUT_CLP_PRIORITIES;

    for (i = 0; i < N_L2_ADDRS; i++)
        m.priorities[i] = default_priority;

    return m;
}


/*
 * MSG_DISABLE_TCP_FRAGMENTATION
 */
MsgDisableTcpFragmentation_t
mf_disable_tcp_fragmentation(void)
{
    MsgDisableTcpFragmentation_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_DISABLE_TCP_FRAGMENTATION;

    return m;
}


/*
 * MSG_INITIALISE_FLOW
 *
 * The exact fields of initialise_flow_t belong to the production
 * implementation, so initialise the complete production structure
 * to zero here.
 */
MsgInitialiseFlow_t
mf_initialise_flow(void)
{
    MsgInitialiseFlow_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_INITIALISE_FLOW;

    return m;
}


/*
 * MSG_RETRIEVE_BUFFER_UTILISATION
 */
MsgRetrieveBufferUtilisation_t
mf_retrieve_buffer_utilisation(void)
{
    MsgRetrieveBufferUtilisation_t m;

    memset(&m, 0, sizeof(m));

    m.id = MSG_RETRIEVE_BUFFER_UTILISATION;

    return m;
}
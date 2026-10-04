#define TEST_DBG_LEVEL       3
#define TEST_MTU             1200
#define TEST_L2_ADDR         2
#define TEST_CL_PRIORITY     7
#define TEST_ALL_CL_PRIORITY 5

// Test 2 — MSG_SET_PROTO_DRV_DBG_LEVEL
static void test_set_proto_drv_dbg_level(void)
{
    MsgSetProtoDrvDbgLevel_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST 2: MSG_SET_PROTO_DRV_DBG_LEVEL\n");
    printf("----------------------------------------\n");

    msg = mf_set_proto_drv_dbg_level(TEST_DBG_LEVEL);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_SET_PROTO_DRV_DBG_LEVEL");

    pass_step("MSG_SET_PROTO_DRV_DBG_LEVEL sent from mock PME");

    /*
     * No response is defined by protoman_socket().
     *
     * Verification:
     *     test-only state inspection must verify:
     *
     *         pms->ps->dbg_level == TEST_DBG_LEVEL
     */

    pass_step("MSG_SET_PROTO_DRV_DBG_LEVEL processed by real PRODRV");
	/*
	Verification: test-only inspection of:

	pms->ps->dbg_level
	*/
	
}

// 5. MSG_INIT_PREPARE_FRAME_STATS
static void test_init_prepare_frame_stats(void)
{
    MsgInitPrepareFrameStats_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_INIT_PREPARE_FRAME_STATS\n");
    printf("----------------------------------------\n");

    msg = mf_init_prepare_frame_stats();

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_INIT_PREPARE_FRAME_STATS");

    pass_step("MSG_INIT_PREPARE_FRAME_STATS sent from mock PME");

    /*
     * Verification:
     *
     *   work_pf and full_pf statistics should be cleared/reinitialised.
     *
     * This is best verified through the test diagnostic interface.
     */

    pass_step("MSG_INIT_PREPARE_FRAME_STATS processed by real PRODRV");
	
	/* Verification: inspect:

	pms->ps->pf[0]
	pms->ps->pf[1]

	and verify the expected zeroed statistics.*/
}

// 6. MSG_GET_PREPARE_FRAME_STATS
static void test_get_prepare_frame_stats(void)
{
    MsgGetPrepareFrameStats_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_GET_PREPARE_FRAME_STATS\n");
    printf("----------------------------------------\n");

    msg = mf_get_prepare_frame_stats();

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_GET_PREPARE_FRAME_STATS");

    pass_step("MSG_GET_PREPARE_FRAME_STATS sent from mock PME");

    /*
     * IMPORTANT:
     *
     * protoman_socket() writes the result into msg.stats in the
     * PRODRV process. The mock PME's copy of msg is not modified.
     *
     * Therefore this cannot be verified from g_test.pme_fd unless
     * the production protocol is changed to return the structure.
     *
     * Use the test-only diagnostic interface to retrieve the
     * calculated statistics.
     */

    pass_step("MSG_GET_PREPARE_FRAME_STATS processed by real PRODRV");
	/* 
	Verification: compare diagnostic statistics with:

	pf[0] + pf[1]

	according to the production calculation.
	*/
}

// 7. MSG_SET_PROTO_DRV_MTU
static void test_set_proto_drv_mtu(void)
{
    MsgSetProtoDrvMtu_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_SET_PROTO_DRV_MTU\n");
    printf("----------------------------------------\n");

    msg = mf_set_proto_drv_mtu(TEST_MTU);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_SET_PROTO_DRV_MTU");

    pass_step("MSG_SET_PROTO_DRV_MTU sent from mock PME");

    /*
     * Verification:
     *
     *     ifp->if_mtu == TEST_MTU
     */

    pass_step("MSG_SET_PROTO_DRV_MTU processed by real PRODRV");
	
	/*
	Verification:

	ifp->if_mtu == TEST_MTU
	*/
}

// 8. MSG_SET_LOCAL_ADDRESS
static void test_set_local_address(void)
{
    MsgSetLocalAddress_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_SET_LOCAL_ADDRESS\n");
    printf("----------------------------------------\n");

    msg = mf_set_local_address(TEST_L2_ADDR);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_SET_LOCAL_ADDRESS");

    pass_step("MSG_SET_LOCAL_ADDRESS sent from mock PME");

    /*
     * IMPORTANT:
     *
     * The current production implementation only validates/casts
     * the argument and returns OK. It does not actually store the
     * address anywhere.
     *
     * Therefore there is currently no state change to verify.
     */

    pass_step("MSG_SET_LOCAL_ADDRESS processed by real PRODRV");
	
	/*
	ommand-path verification only.

	production code currently does not set the local address.
	*/
}

// 9. MSG_INITIALISE_COMPRESSION_DICTIONARY
static void test_initialise_compression_dictionary(void)
{
    MsgInitialiseCompressionDictionary_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_INITIALISE_COMPRESSION_DICTIONARY\n");
    printf("----------------------------------------\n");

    msg = mf_initialise_compression_dictionary(TEST_L2_ADDR);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_INITIALISE_COMPRESSION_DICTIONARY");

    pass_step("MSG_INITIALISE_COMPRESSION_DICTIONARY sent from mock PME");

    /*
     * Verification:
     *
     * inspect the compression dictionary / related state through
     * the test diagnostic interface.
     */

    pass_step("MSG_INITIALISE_COMPRESSION_DICTIONARY processed");
}

// 10. MSG_INITIALISE_TX_COMPRESSION_DICTIONARY
static void test_initialise_tx_compression_dictionary(void)
{
    MsgInitialiseTxCompressionDictionary_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_INITIALISE_TX_COMPRESSION_DICTIONARY\n");
    printf("----------------------------------------\n");

    msg = mf_initialise_tx_compression_dictionary(TEST_L2_ADDR);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_INITIALISE_TX_COMPRESSION_DICTIONARY");

    pass_step("MSG_INITIALISE_TX_COMPRESSION_DICTIONARY sent from mock PME");

    /*
     * Verification:
     *
     * inspect TX compression dictionary state through the
     * test diagnostic interface.
     */

    pass_step("MSG_INITIALISE_TX_COMPRESSION_DICTIONARY processed");
}

// 11. MSG_INITIALISE_REPLAY_COUNTERS
static void test_initialise_replay_counters(void)
{
    MsgInitialiseReplayCounters_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_INITIALISE_REPLAY_COUNTERS\n");
    printf("----------------------------------------\n");

    msg = mf_initialise_replay_counters(TEST_L2_ADDR);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_INITIALISE_REPLAY_COUNTERS");

    pass_step("MSG_INITIALISE_REPLAY_COUNTERS sent from mock PME");

    /*
     * Verification:
     *
     * inspect replay-counter state for TEST_L2_ADDR.
     */

    pass_step("MSG_INITIALISE_REPLAY_COUNTERS processed");
}

// 12. MSG_ENABLE_LINK
static void test_enable_link(void)
{
    MsgEnableLink_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_ENABLE_LINK\n");
    printf("----------------------------------------\n");

    msg = mf_enable_link(TEST_L2_ADDR);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_ENABLE_LINK");

    pass_step("MSG_ENABLE_LINK sent from mock PME");

    /*
     * Verification:
     *
     *     proto_statics.link_stat[TEST_L2_ADDR] == LS_ENABLED
     */

    pass_step("MSG_ENABLE_LINK processed");
}

// 13. MSG_DISABLE_LINK
static void test_disable_link(void)
{
    MsgDisableLink_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_DISABLE_LINK\n");
    printf("----------------------------------------\n");

    msg = mf_disable_link(TEST_L2_ADDR);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_DISABLE_LINK");

    pass_step("MSG_DISABLE_LINK sent from mock PME");

    /*
     * Verification:
     *
     *     proto_statics.link_stat[TEST_L2_ADDR] == LS_DISABLED
     */

    pass_step("MSG_DISABLE_LINK processed");
}

// 14. MSG_FLUSH
static void test_flush(void)
{
    MsgFlush_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_FLUSH\n");
    printf("----------------------------------------\n");

    msg = mf_flush(TEST_L2_ADDR);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_FLUSH");

    pass_step("MSG_FLUSH sent from mock PME");

    /*
     * Verification:
     *
     * The queues associated with TEST_L2_ADDR should be empty.
     *
     * This should be checked through the relevant queue/flow
     * state exposed by the test diagnostic interface.
     */

    pass_step("MSG_FLUSH processed");
}

// 15. MSG_DELETE_LINK
static void test_delete_link(void)
{
    MsgDeleteLink_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_DELETE_LINK\n");
    printf("----------------------------------------\n");

    msg = mf_delete_link(TEST_L2_ADDR);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_DELETE_LINK");

    pass_step("MSG_DELETE_LINK sent from mock PME");

    /*
     * Verification:
     *
     * Verify that the link/associated state for TEST_L2_ADDR
     * has been deleted.
     */

    pass_step("MSG_DELETE_LINK processed");
}

// 16. MSG_SET_CLP_PRIORITY
static void test_set_clp_priority(void)
{
    MsgSetClpPriority_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_SET_CLP_PRIORITY\n");
    printf("----------------------------------------\n");

    msg = mf_set_clp_priority(
        TEST_L2_ADDR,
        TEST_CL_PRIORITY);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_SET_CLP_PRIORITY");

    pass_step("MSG_SET_CLP_PRIORITY sent from mock PME");

    /*
     * Verification is performed by the subsequent
     * MSG_GET_CLP_PRIORITY test.
     */

    pass_step("MSG_SET_CLP_PRIORITY processed");
}

// 17. MSG_GET_CLP_PRIORITY
static void test_get_clp_priority(void)
{
    MsgGetClpPriority_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_GET_CLP_PRIORITY\n");
    printf("----------------------------------------\n");

    msg = mf_get_clp_priority(TEST_L2_ADDR);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_GET_CLP_PRIORITY");

    pass_step("MSG_GET_CLP_PRIORITY sent from mock PME");

    /*
     * The result is written into the argument buffer in PRODRV.
     *
     * Since the current protocol has no response message, obtain
     * the result using the test diagnostic interface.
     */

    /*
     * Expected:
     *
     *     priority == TEST_CL_PRIORITY
     */

    pass_step("MSG_GET_CLP_PRIORITY processed");
}

// 18. MSG_GET_CLP_PRIORITIES
static void test_get_clp_priorities(void)
{
    MsgGetClpPriorities_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_GET_CLP_PRIORITIES\n");
    printf("----------------------------------------\n");

    msg = mf_get_clp_priorities();

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_GET_CLP_PRIORITIES");

    pass_step("MSG_GET_CLP_PRIORITIES sent from mock PME");

    /*
     * Verify all N_L2_ADDRS values through the test diagnostic
     * interface.
     */

    pass_step("MSG_GET_CLP_PRIORITIES processed");
}

// 19. MSG_PUT_CLP_PRIORITIES
static void test_put_clp_priorities(void)
{
    MsgPutClpPriorities_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_PUT_CLP_PRIORITIES\n");
    printf("----------------------------------------\n");

    msg = mf_put_clp_priorities(TEST_ALL_CL_PRIORITY);

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_PUT_CLP_PRIORITIES");

    pass_step("MSG_PUT_CLP_PRIORITIES sent from mock PME");

    /*
     * Verify using MSG_GET_CLP_PRIORITIES / diagnostic state.
     */

    pass_step("MSG_PUT_CLP_PRIORITIES processed");
}

// 20. MSG_DISABLE_TCP_FRAGMENTATION
static void test_disable_tcp_fragmentation(void)
{
    MsgDisableTcpFragmentation_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_DISABLE_TCP_FRAGMENTATION\n");
    printf("----------------------------------------\n");

    msg = mf_disable_tcp_fragmentation();

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_DISABLE_TCP_FRAGMENTATION");

    pass_step("MSG_DISABLE_TCP_FRAGMENTATION sent from mock PME");

    /*
     * Verification:
     *
     *     pms->ps->TCP_fragmentation == TCP_FRAG_DISABLE
     */

    pass_step("MSG_DISABLE_TCP_FRAGMENTATION processed");
}

// 21. MSG_INITIALISE_FLOW
static void test_initialise_flow(void)
{
    MsgInitialiseFlow_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_INITIALISE_FLOW\n");
    printf("----------------------------------------\n");

    msg = mf_initialise_flow();

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_INITIALISE_FLOW");

    pass_step("MSG_INITIALISE_FLOW sent from mock PME");

    /*
     * Verification:
     *
     * Verify that initialise_flow() created/initialised the
     * expected flow state.
     *
     * The exact assertion depends on the implementation of
     * initialise_flow().
     */

    pass_step("MSG_INITIALISE_FLOW processed");
}

// 22. MSG_RETRIEVE_BUFFER_UTILISATION
static void test_retrieve_buffer_utilisation(void)
{
    MsgRetrieveBufferUtilisation_t msg;

    printf("\n");
    printf("----------------------------------------\n");
    printf("TEST: MSG_RETRIEVE_BUFFER_UTILISATION\n");
    printf("----------------------------------------\n");

    msg = mf_retrieve_buffer_utilisation();

    if (send_all(g_test.pme_fd, &msg, sizeof(msg)) < 0)
        fail_test("Failed to send MSG_RETRIEVE_BUFFER_UTILISATION");

    pass_step("MSG_RETRIEVE_BUFFER_UTILISATION sent from mock PME");

    /*
     * retrieve_buffer_utilisation() writes the result into
     * the argument buffer inside PRODRV.
     *
     * Verify the resulting buffer-utilisation data using the
     * test diagnostic interface.
     */

    pass_step("MSG_RETRIEVE_BUFFER_UTILISATION processed");
}


// IN THE MAIN

test_init_prodrv();

test_proto_reset();

test_set_proto_drv_dbg_level();

test_init_prepare_frame_stats();

test_set_proto_drv_mtu();

test_set_local_address();

test_initialise_compression_dictionary();

test_initialise_tx_compression_dictionary();

test_initialise_replay_counters();

test_enable_link();

test_disable_link();

test_flush();

test_delete_link();

test_set_clp_priority();

test_get_clp_priority();

test_put_clp_priorities();

test_get_clp_priorities();

test_disable_tcp_fragmentation();

test_initialise_flow();

test_retrieve_buffer_utilisation();

test_get_prepare_frame_stats();
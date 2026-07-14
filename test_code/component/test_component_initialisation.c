/*=====================================================================
 *
 * File: test_component_initialisation.c
 *
 * Component Test:
 *      CTRL ----MSG_INIT_PRODRV----> PRODRV
 *      PRODRV ----MSG_ACK---------> SLOTMGR
 *
 *====================================================================*/

#include "unity.h"

#include "proto_handlers.h"
#include "test_component_common.h"

#include "mock_logger.h"

void setUp(void)
{
    component_setup();

    mock_logger_reset();
}

void tearDown(void)
{
    component_teardown();
}

void test_handle_MSG_INIT_PRODRV_updates_context(void)
{
    MsgInitProdrv_t msg = mf_init_prodrv();

    handle_MSG_INIT_PRODRV(&g_fixture.ctx,
                           (uint8_t *)&msg);

    TEST_ASSERT_TRUE(g_fixture.ctx.init_received);

    TEST_ASSERT_EQUAL(msg.chunk1_bytes,
                      g_fixture.ctx.config.frame_params->N1);

    TEST_ASSERT_EQUAL(msg.prepare_l2_frame_to_tx_ms,
                      g_fixture.ctx.config.frame_params->N2);
}

void test_send_MSG_ACK_INIT_PRODRV_generates_correct_packet(void)
{
    MsgAck_t ack;

    socketpair(AF_UNIX,
               SOCK_STREAM,
               0,
               (int[2]){0});

    int sv[2];

    TEST_ASSERT_EQUAL(0,
                      socketpair(AF_UNIX,
                                 SOCK_STREAM,
                                 0,
                                 sv));

    g_fixture.ctx.slotmgr_sock_fd = sv[0];

    send_MSG_ACK_init_prodrv(&g_fixture.ctx);

    TEST_ASSERT_EQUAL(sizeof(MsgAck_t),
                      recv(sv[1],
                           &ack,
                           sizeof(ack),
                           0));

    TEST_ASSERT_EQUAL(MSG_ACK,
                      ack.id);

    TEST_ASSERT_EQUAL(MSG_INIT_PRODRV,
                      ack.ack_msg);

    TEST_ASSERT_EQUAL(g_fixture.ctx.hcs_index,
                      ack.hcs_index);

    close(sv[0]);
    close(sv[1]);
}

void test_handle_MSG_INIT_PRODRV_sets_initialised_flag(void)
{
    MsgInitProdrv_t msg = mf_init_prodrv();

    g_fixture.ctx.init_received = false;

    handle_MSG_INIT_PRODRV(&g_fixture.ctx,
                           (uint8_t *)&msg);

    TEST_ASSERT_TRUE(g_fixture.ctx.init_received);
}

void test_handle_MSG_INIT_PRODRV_invalid_chunk_size_still_initialises(void)
{
    MsgInitProdrv_t msg = mf_init_prodrv();

    msg.chunk1_bytes = 0xffff;

    handle_MSG_INIT_PRODRV(&g_fixture.ctx,
                           (uint8_t *)&msg);

    TEST_ASSERT_TRUE(g_fixture.ctx.init_received);

    TEST_ASSERT_GREATER_THAN(0,
                             mock_log_count);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_handle_MSG_INIT_PRODRV_updates_context);

    RUN_TEST(test_send_MSG_ACK_INIT_PRODRV_generates_correct_packet);

    RUN_TEST(test_handle_MSG_INIT_PRODRV_sets_initialised_flag);

    RUN_TEST(test_handle_MSG_INIT_PRODRV_invalid_chunk_size_still_initialises);

    return UNITY_END();
}
```
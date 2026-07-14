/*=====================================================================
 *
 * File: test_component_prepare_frame.c
 *
 * Component Test:
 *
 *      SlotMgr
 *          |
 *          | MSG_PREPARE_FRAME
 *          V
 *      PRODRV
 *          |
 *          +--> make_L2frame_ng()
 *          |
 *          +--> write_chunk2CSH()
 *          |
 *          +--> MSG_TX_START
 *          |
 *          V
 *      Mock CSH Event Handler
 *
 *====================================================================*/

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "unity.h"

#include "proto_handlers.h"
#include "proto_main.h"

#include "message_factory.h"

#include "mock_makeframe.h"
#include "mock_logger.h"

#include "test_component_common.h"

extern int mock_writechunk_calls;
extern int mock_last_writechunk_frame_len;
extern int mock_last_writechunk_chunks;

void setUp(void)
{
    component_setup();

    mock_makeframe_reset();

    mock_logger_reset();
}

void tearDown(void)
{
    component_teardown();
}

/*********************************************************************
 *
 * Successful frame preparation
 *
 *********************************************************************/

void test_prepare_frame_success(void)
{
    MsgPrepareFrame_t msg;

    msg = mf_prepare_frame();

    mock_makeframe_return = 0;

    mock_frame_length = 280;

    TEST_ASSERT_EQUAL(
        0,
        handle_MSG_PREPARE_FRAME(
            g_fixture.ifp,
            &g_fixture.ctx,
            (uint8_t *)&msg));

    TEST_ASSERT_EQUAL(
        1,
        mock_makeframe_calls);

    TEST_ASSERT_EQUAL(
        1,
        mock_writechunk_calls);
}

/*********************************************************************
 *
 * make_L2frame() failure
 *
 *********************************************************************/

void test_prepare_frame_makeframe_failure(void)
{
    MsgPrepareFrame_t msg;

    msg = mf_prepare_frame();

    mock_makeframe_return = -12;

    TEST_ASSERT_EQUAL(
        -1,
        handle_MSG_PREPARE_FRAME(
            g_fixture.ifp,
            &g_fixture.ctx,
            (uint8_t *)&msg));

    TEST_ASSERT_EQUAL(
        1,
        mock_makeframe_calls);

    TEST_ASSERT_EQUAL(
        0,
        mock_writechunk_calls);
}

/*********************************************************************
 *
 * Transaction ID propagated
 *
 *********************************************************************/

void test_prepare_frame_transaction_id_preserved(void)
{
    MsgPrepareFrame_t prepare;

    MsgTxStart_t tx;

    int sv[2];

    TEST_ASSERT_EQUAL(
        0,
        socketpair(AF_UNIX,
                   SOCK_STREAM,
                   0,
                   sv));

    g_fixture.ctx.csh_evt_sock_fd = sv[0];

    prepare = mf_prepare_frame();

    prepare.tr_id = 0x4321;

    TEST_ASSERT_EQUAL(
        0,
        handle_MSG_PREPARE_FRAME(
            g_fixture.ifp,
            &g_fixture.ctx,
            (uint8_t *)&prepare));

    recv(sv[1],
         &tx,
         sizeof(tx),
         0);

    TEST_ASSERT_EQUAL(
        MSG_TX_START,
        tx.id);

    TEST_ASSERT_EQUAL(
        prepare.tr_id,
        tx.tr_id);

    close(sv[0]);

    close(sv[1]);
}

/*********************************************************************
 *
 * Schedule metadata copied
 *
 *********************************************************************/

void test_prepare_frame_schedule_copied(void)
{
    MsgPrepareFrame_t prepare;

    MsgTxStart_t tx;

    int sv[2];

    socketpair(AF_UNIX,
               SOCK_STREAM,
               0,
               sv);

    g_fixture.ctx.csh_evt_sock_fd = sv[0];

    prepare = mf_prepare_frame();

    prepare.sch_meta.lrtc = 998877;

    prepare.sch_meta.radio_freq = 456789;

    prepare.sch_meta.radio_power = HIGH;

    TEST_ASSERT_EQUAL(
        0,
        handle_MSG_PREPARE_FRAME(
            g_fixture.ifp,
            &g_fixture.ctx,
            (uint8_t *)&prepare));

    recv(sv[1],
         &tx,
         sizeof(tx),
         0);

    TEST_ASSERT_EQUAL(
        prepare.sch_meta.lrtc,
        tx.sch_meta.lrtc);

    TEST_ASSERT_EQUAL(
        prepare.sch_meta.radio_freq,
        tx.sch_meta.radio_freq);

    TEST_ASSERT_EQUAL(
        prepare.sch_meta.radio_power,
        tx.sch_meta.radio_power);

    close(sv[0]);

    close(sv[1]);
}

/*********************************************************************
 *
 * Payload cleared before copy
 *
 *********************************************************************/

void test_prepare_frame_payload_zero_initialised(void)
{
    MsgPrepareFrame_t prepare;

    MsgTxStart_t tx;

    int sv[2];

    socketpair(AF_UNIX,
               SOCK_STREAM,
               0,
               sv);

    g_fixture.ctx.csh_evt_sock_fd = sv[0];

    prepare = mf_prepare_frame();

    TEST_ASSERT_EQUAL(
        0,
        handle_MSG_PREPARE_FRAME(
            g_fixture.ifp,
            &g_fixture.ctx,
            (uint8_t *)&prepare));

    recv(sv[1],
         &tx,
         sizeof(tx),
         0);

    TEST_ASSERT_NOT_EQUAL(
        0,
        tx.tx_payload[0]);

    close(sv[0]);

    close(sv[1]);
}

/*********************************************************************
 *
 * Statistics updated
 *
 *********************************************************************/

void test_prepare_frame_statistics_incremented(void)
{
    MsgPrepareFrame_t msg;

    struct proto_statics *ps;

    ps = (struct proto_statics *)g_fixture.ifp->p;

    msg = mf_prepare_frame();

    ps->work_pf->frames_prepared = 0;

    TEST_ASSERT_EQUAL(
        0,
        handle_MSG_PREPARE_FRAME(
            g_fixture.ifp,
            &g_fixture.ctx,
            (uint8_t *)&msg));

    TEST_ASSERT_EQUAL(
        1,
        ps->work_pf->frames_prepared);

    TEST_ASSERT_EQUAL(
        1,
        ps->work_pf->chunks_sent);
}

/*********************************************************************
 *
 * Multiple prepare frame requests
 *
 *********************************************************************/

void test_prepare_frame_multiple_requests(void)
{
    MsgPrepareFrame_t msg;

    msg = mf_prepare_frame();

    mock_makeframe_return = 0;

    for(int i=0;i<20;i++)
    {
        TEST_ASSERT_EQUAL(
            0,
            handle_MSG_PREPARE_FRAME(
                g_fixture.ifp,
                &g_fixture.ctx,
                (uint8_t *)&msg));
    }

    TEST_ASSERT_EQUAL(
        20,
        mock_makeframe_calls);

    TEST_ASSERT_EQUAL(
        20,
        mock_writechunk_calls);
}

/*********************************************************************
 *
 * write_chunk2CSH receives expected frame length
 *
 *********************************************************************/

void test_prepare_frame_correct_frame_length(void)
{
    MsgPrepareFrame_t msg;

    msg = mf_prepare_frame();

    mock_frame_length = 1024;

    TEST_ASSERT_EQUAL(
        0,
        handle_MSG_PREPARE_FRAME(
            g_fixture.ifp,
            &g_fixture.ctx,
            (uint8_t *)&msg));

    TEST_ASSERT_EQUAL(
        1024,
        mock_last_writechunk_frame_len);
}

/*********************************************************************
 *
 * Unity runner
 *
 *********************************************************************/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_prepare_frame_success);

    RUN_TEST(test_prepare_frame_makeframe_failure);

    RUN_TEST(test_prepare_frame_transaction_id_preserved);

    RUN_TEST(test_prepare_frame_schedule_copied);

    RUN_TEST(test_prepare_frame_payload_zero_initialised);

    RUN_TEST(test_prepare_frame_statistics_incremented);

    RUN_TEST(test_prepare_frame_multiple_requests);

    RUN_TEST(test_prepare_frame_correct_frame_length);

    return UNITY_END();
}
```

````c
/*=====================================================================
 *
 * File: mock_writechunk.h
 *
 *====================================================================*/

#ifndef MOCK_WRITECHUNK_H
#define MOCK_WRITECHUNK_H

#include <stdint.h>

extern int mock_writechunk_calls;

extern int mock_writechunk_return;

extern int mock_last_writechunk_chunks;

extern int mock_last_writechunk_frame_len;

void mock_writechunk_reset(void);

#endif
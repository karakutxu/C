Using message_factory
---------------------
Your message_factory is already almost perfect.

For example

MsgPrepareFrame_t msg = mf_prepare_frame();

send(slotmgr_fd,
     &msg,
     sizeof(msg),
     0);

MsgTxStart_t tx;

recv(csh_fd,
     &tx,
     sizeof(tx),
     0);

TEST_ASSERT_EQUAL(MSG_TX_START,
                  tx.id);

TEST_ASSERT_EQUAL(msg.tr_id,
                  tx.tr_id);

That becomes incredibly readable.


Using socket_test_utils
-----------------------
I would extend it slightly.

Add

int send_message(int fd,
                 const void *msg,
                 size_t len);

int recv_message(int fd,
                 void *msg,
                 size_t len);

bool wait_for_socket(int fd,
                     int timeout_ms);

bool expect_no_message(int fd,
                       int timeout_ms);

Those four helpers remove hundreds of repeated lines across the suite.
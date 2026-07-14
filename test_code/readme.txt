The component tests use real

AF_UNIX sockets
poll()
accept()
read_socket()
send()

and mock only the external modules.     

                 +----------------------+
                 |      Unity Test      |
                 +----------+-----------+
                            |
                            |
              creates real Unix sockets
                            |
       +--------------------+--------------------+
       |                    |                    |
       |                    |                    |
+------+-----+      +-------+------+     +-------+------+
| Mock CTRL  |      |   PRODRV      |     | Mock SlotMgr |
+------------+      | (real code)   |     +--------------+
                    | poll()         |
                    | dispatcher     |
                    | handlers       |
                    +-------+--------+
                            |
                            |
                     +------+------+
                     |  Mock CSH   |
                     +-------------+


External modules to mock
--------------------------

Everything outside PRODRV.

make_L2frame_ng()

write_chunk2CSH()

read_CSHchunk_ng()

protoman_socket()

tun_alloc()

mbuf_from_buffer()

proto_output_ng()

client_wait_to_conn()

get_server_filepath()

LOG()

Everything else should be real.



Component Test Suite
--------------------

suite split into independent executables.

component/

    test_component_initialisation.c

    test_component_prepare_frame.c

    test_component_read_chunk.c

    test_component_control_messages.c

    test_component_dispatch.c

    test_component_poll.c

    test_component_tun_rx.c

    test_component_invalid_messages.c

Each executable has its own fixture.



Test Fixture
------------

Every test uses exactly the same setup.

TEST_SETUP()
{

create socket directory

start mock CTRL

start mock SlotMgr

start mock CSH Event

start mock CSH RX

launch PRODRV thread

wait until sockets connect

}

Every test tears everything down afterwards.
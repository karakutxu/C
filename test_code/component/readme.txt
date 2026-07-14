Component tests

These use real AF_UNIX sockets.

Nothing is mocked except

make_L2frame()
read_CSHchunk()
protoman_socket()

**********************************************************************
Component Test 1
----------------
Initialisation

Sequence

CTRL

↓

MSG_INIT_PRODRV

↓

PRODRV

↓

handle_MSG_INIT_PRODRV()

↓

MSG_ACK

↓

SlotMgr

Assertions

ACK received

ACK.ack_msg==MSG_INIT_PRODRV

correct HCS index

connection still alive

poll loop continues


Component Test 2
----------------

Prepare Frame

SlotMgr sends

MSG_PREPARE_FRAME

Expected

dispatch_message()

↓

handle_MSG_PREPARE_FRAME()

↓

make_L2frame()

↓

write_chunk2CSH()

↓

MSG_TX_START

↓

Mock CSH

Assertions

make_L2frame called once

write_chunk called once

MSG_TX_START received

transaction id preserved

schedule preserved

Component Test 3
----------------
Read Chunk

Mock CSH RX sends

MSG_READ_CSH_CHUNK

Expected

read_CSHchunk_ng()

called exactly once

Component Test 4
----------------
Control Messages

For every control command

ENABLE_LINK

DISABLE_LINK

DELETE_LINK

FLUSH

SET_LOCAL_ADDRESS

...


Expected

dispatcher

↓

handle_MSG_PROTOMAN_CONTROL()

↓

protoman_socket()


Verify

correct MsgID

correct arguments

called once

Component Test 5
-----------------
Unexpected Socket

Send

MSG_PREPARE_FRAME

through CTRL socket.

Expected

validate_source_socket()

↓

LOG error

↓

still dispatch

(or whatever behaviour you decide)

Component Test 6
----------------
Unknown Message

Send

0xffff

Expected

LOG warning

no crash

poll continues

Component Test 7
----------------
Disconnect

Close SlotMgr socket.

Expected

poll

↓

POLLHUP

↓

error handling

↓

shutdown

or

↓

connection removed

Component Test 8
----------------
Multiple Clients

Create

CTRL

SlotMgr

CSH EVT

CSH RX

Extra client

Verify

accept_client()

works

poll list updated

no fd corruption
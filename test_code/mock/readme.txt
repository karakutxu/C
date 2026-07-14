Mock Architecture
-------------------
 keep mocks extremely small.

Example

mock_makeframe.c

----------------

int makeframe_calls;

int makeframe_return;

int make_L2frame_ng(...)
{
    makeframe_calls++;

    return makeframe_return;
}

Then tests simply write

makeframe_return=0;

or

makeframe_return=-17;

No complicated framework required.


Mock Socket Peers
--------------------
The peer mocks should behave like tiny servers.

Example

Mock SlotMgr

↓

accept()

↓

wait for ACK

↓

verify packet

↓

exit

No business logic.

Just protocol verification.
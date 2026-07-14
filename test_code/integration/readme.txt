Integration tests

These exercise the complete module.

For example

CTRL

↓

MSG_INIT_PRODRV

↓

PRODRV

↓

MSG_ACK

↓

SlotMgr

		or

SlotMgr

↓

MSG_PREPARE_FRAME

↓

PRODRV

↓

make_L2frame()

↓

write_chunk2CSH()

↓

MSG_TX_START

↓

mock CSH
Static functions

Perfectly fine.

Example:

static void handle_prepare_frame(...)
{
}

No issue.

Each process has its own copy.

********************************
What I would actually implement

For each interface:

prodrv_0 process
prodrv_1 process
prodrv_2 process

each process owns:

struct prodrv_ctx
{
    ...
    struct ifnet ifp;
    struct proto_statics proto;
};

and all protocol state lives there.

Then:

read_VMEchunk(&ctx->ifp,...);
make_L2frame(&ctx->ifp,...);
proto_output(&ctx->ifp,...);

operate exactly like LynxOS did, but without hidden global state.

That will make replay testing and multi-interface operation much more predictable and will eliminate a large class of bugs during the port.

********************************
Instead I'd structure it around:

single process main loop
AF_UNIX server
outgoing client sockets
poll() dispatch
internal context/state
message handlers
small TX/RX virtual interfaces

Something closer to:



That preserves your existing AF_UNIX socket/message architecture, keeps compatibility with the rest of PME/SLOTMGR/CSH, and isolates all legacy DPR/L2 frame code behind a small virtual interface so you can replace it later without touching the main event loop.

with the main process being socket-driven and all Layer-2 implementation hidden behind driver callbacks.

This is the structure I'd recommend as the "tidied" PRODRV equivalent of the newer SlotMgr style: a single event-driven process, socket-based IPC, centralized context, dispatcher-based message handling, and driver abstraction for the legacy Layer-2 code.
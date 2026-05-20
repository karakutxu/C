int main(void)
{
    struct proto_context ctx;

    memset(&ctx, 0, sizeof(ctx));

    /*
     * Fake scheduler
     */
    int slot_pipe[2];
    pipe(slot_pipe);

    /*
     * Fake RX path
     */
    int rx_sock[2];
    socketpair(AF_UNIX, SOCK_DGRAM, 0, rx_sock);

    /*
     * Fake control plane
     */
    int ctrl_sock[2];
    socketpair(AF_UNIX, SOCK_DGRAM, 0, ctrl_sock);

    ctx.slotmgr_fd = slot_pipe[0];
    ctx.rx_fd       = rx_sock[0];
    ctx.ctrl_fd     = ctrl_sock[0];

    /*
     * Optional fake tun
     */
    ctx.tun_fd = create_fake_tun();

    /*
     * Start protocol loop thread
     */
    pthread_t tid;

    pthread_create(&tid,
                   NULL,
                   proto_thread,
                   &ctx);

    /*
     * Inject scheduler event
     */
    write(slot_pipe[1], "T", 1);

    /*
     * Inject RX chunk
     */
    struct RX_LAYER_2_CHUNK chunk;

    load_captured_chunk(&chunk);

    send(rx_sock[1],
         &chunk,
         sizeof(chunk),
         0);

    /*
     * Inject control command
     */
    send(ctrl_sock[1],
         "RESET",
         5,
         0);

    pthread_join(tid, NULL);

    return 0;
}
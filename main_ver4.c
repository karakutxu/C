int main(int argc, char **argv)
{
    struct proto_context ctx;

    memset(&ctx, 0, sizeof(ctx));

    /*
     * Init protocol subsystem
     */
    proto_init(&ctx);

    /*
     * Create tun interface
     */
    ctx.tun_fd = tun_alloc("halo0");

    /*
     * Connect to slot manager
     */
    ctx.slotmgr_fd = connect_slot_manager();

    /*
     * Connect to control plane
     */
    ctx.ctrl_fd = connect_protoman();

    /*
     * RX chunk source
     */
    ctx.rx_fd = connect_rx_pipeline();

    /*
     * Run protocol loop
     */
    proto_main_loop(&ctx);

    return 0;
}
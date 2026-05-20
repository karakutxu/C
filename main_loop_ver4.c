void proto_main_loop(struct proto_context *ctx)
{
    fd_set rfds;

    for (;;)
    {
        FD_ZERO(&rfds);

        /*
         * TX packet ingress
         */
        FD_SET(ctx->tun_fd, &rfds);

        /*
         * Slot manager scheduler trigger
         */
        FD_SET(ctx->slotmgr_fd, &rfds);

        /*
         * RX chunk ingress
         */
        FD_SET(ctx->rx_fd, &rfds);

        /*
         * PROTOMAN control plane
         */
        FD_SET(ctx->ctrl_fd, &rfds);

        int maxfd = max4(
            ctx->tun_fd,
            ctx->slotmgr_fd,
            ctx->rx_fd,
            ctx->ctrl_fd);

        int ret = select(maxfd + 1,
                         &rfds,
                         NULL,
                         NULL,
                         NULL);

        if (ret < 0)
        {
            perror("select");
            continue;
        }

        /*
         * TX ingress from TUN
         */
        if (FD_ISSET(ctx->tun_fd, &rfds))
        {
            handle_tun_ingress(ctx);
        }

        /*
         * Scheduler TX trigger
         */
        if (FD_ISSET(ctx->slotmgr_fd, &rfds))
        {
            handle_scheduler_trigger(ctx);
        }

        /*
         * Incoming RX chunk
         */
        if (FD_ISSET(ctx->rx_fd, &rfds))
        {
            handle_rx_chunk(ctx);
        }

        /*
         * PROTOMAN control commands
         */
        if (FD_ISSET(ctx->ctrl_fd, &rfds))
        {
            handle_control_plane(ctx);
        }
    }
}
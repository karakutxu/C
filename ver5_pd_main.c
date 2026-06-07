#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

#include "unix_sock.h"
#include "logger.h"

#include "prodrv_main.h"
#include "prodrv_handlers.h"

static int connect_remotes(
        ProdrvContext_t *ctx)
{
    ctx->pme_fd =
        client_wait_to_conn(
            ctx->proc_name,
            PME_SOCK);

    ctx->slotmgr_fd =
        client_wait_to_conn(
            ctx->proc_name,
            SLOTMGR_SOCK);

    ctx->csh_evt_fd =
        client_wait_to_conn(
            ctx->proc_name,
            get_server_filepath(
                CSH_EVT_HDLR,
                ctx->hcs_index));

    ctx->csh_proc_rx_fd =
        client_wait_to_conn(
            ctx->proc_name,
            get_server_filepath(
                CSH_PROC_RX,
                ctx->hcs_index));

    return 0;
}

static void dispatch_message(
        ProdrvContext_t *ctx,
        uint8_t *buf)
{
    MsgID_t msg_id =
        get_msg_id(buf);

    switch(msg_id)
    {
        case MSG_INIT_PRODRV:
            handle_msg_init_prodrv(
                ctx,
                buf);
            break;

        case MSG_COMM_PARAM:
            handle_msg_comm_param(
                ctx,
                buf);
            break;

        case MSG_PREPARE_FRAME:
            handle_msg_prepare_frame(
                ctx,
                buf);
            break;

        case MSG_TX_STATS:
            handle_msg_tx_stats(
                ctx,
                buf);
            break;

        case MSG_RX_STATS:
            handle_msg_rx_stats(
                ctx,
                buf);
            break;

        case MSG_DEMOD_DATA:
            handle_msg_demod_data(
                ctx,
                buf);
            break;

        default:
            HALO_NG_LOG(
                WARNING,
                LOC,
                format("[%s] unknown msg=%d",
                ctx->proc_name,
                msg_id));
            break;
    }
}

int prodrv_main(uint8_t hcs_index)
{
    ProdrvContext_t ctx;

    memset(&ctx, 0, sizeof(ctx));

    ctx.hcs_index = hcs_index;

    get_proc_name(
        PRODRV_MAIN,
        ctx.proc_name,
        hcs_index);

    ctx.server_fd =
        sockets_create_local_server_socket(
            get_server_filepath(
                PRODRV_MAIN,
                hcs_index));

    connect_remotes(&ctx);

    ctx.pollfds[0].fd =
        ctx.server_fd;

    ctx.pollfds[0].events =
        POLLIN;

    ctx.n_pollfds = 1;

    uint8_t rbuf[BUF_LEN];

    while (true)
    {
        int rc =
            poll(
                ctx.pollfds,
                ctx.n_pollfds,
                -1);

        if (rc <= 0)
        {
            continue;
        }

        int nfds =
            ctx.n_pollfds;

        for (int i = 0; i < nfds; i++)
        {
            if (!(ctx.pollfds[i].revents & POLLIN))
            {
                continue;
            }

            if (ctx.pollfds[i].fd ==
                ctx.server_fd)
            {
                accept_client(
                    ctx.proc_name,
                    ctx.server_fd,
                    get_server_filepath(
                        PRODRV_MAIN,
                        hcs_index),
                    ctx.pollfds,
                    &ctx.n_pollfds);

                continue;
            }

            int nbytes =
                read_socket(
                    ctx.pollfds[i].fd,
                    rbuf);

            if (nbytes > 0)
            {
                dispatch_message(
                    &ctx,
                    rbuf);
            }
        }
    }

    return 0;
}
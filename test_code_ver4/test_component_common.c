#include "test_component_common.h"
#include <stdlib.h>
#include <string.h>

test_fixture_t g_fixture;

/*
Ensure your common test setup fixture (g_fixture) initializes all pointers expected by handle_MSG_PREPARE_FRAME:

void component_setup(void)
{
    memset(&g_fixture, 0, sizeof(g_fixture));

    // 1. Allocate / attach protocol statics
    g_fixture.s = calloc(1, sizeof(struct proto_statics));
    g_fixture.s->work_pf = &g_fixture.s->pf[0];
    g_fixture.s->full_pf = &g_fixture.s->pf[1];

    // 2. Attach ifnet
    g_fixture.ifp = calloc(1, sizeof(struct ifnet));
    g_fixture.ifp->p = g_fixture.s;
    g_fixture.s->ifp = g_fixture.ifp;

    // 3. Attach frame params & chunk structs
    g_fixture.ctx.config.frame_params = calloc(1, sizeof(struct TX_L2_FRAME_PARAMS));
    g_fixture.ctx.config.wchunk = calloc(1, sizeof(struct TX_LAYER_2_CHUNK));
    g_fixture.ctx.config.chunksize = calloc(1, sizeof(struct TX_L2_CHUNK_SIZE));
}
*/

void component_setup(void)
{
    memset(&g_fixture, 0, sizeof(test_fixture_t));

    g_fixture.ifp = calloc(1, sizeof(struct ifnet));
    
    g_fixture.statics.work_pf = &g_fixture.work_stats;
    g_fixture.statics.full_pf = &g_fixture.full_stats;
    g_fixture.statics.ifp = g_fixture.ifp;

    g_fixture.ifp->p = &g_fixture.statics;

    g_fixture.ctx.config.frame_params = calloc(1, sizeof(struct TX_L2_FRAME_PARAMS));
    g_fixture.ctx.config.wchunk = calloc(1, sizeof(struct TX_LAYER_2_CHUNK));
    g_fixture.ctx.config.chunksize = calloc(1, sizeof(struct TX_L2_CHUNK_SIZE));

    g_fixture.ctx.csh_evt_sock_fd = -1;
    g_fixture.ctx.ctrl_sock_fd = -1;
    g_fixture.ctx.slotmgr_sock_fd = -1;
    g_fixture.ctx.csh_proc_rx_sock_fd = -1;
}

void component_teardown(void)
{
    if (g_fixture.ifp != NULL)
    {
        free(g_fixture.ifp);
        g_fixture.ifp = NULL;
    }

    if (g_fixture.ctx.config.frame_params != NULL)
    {
        free(g_fixture.ctx.config.frame_params);
        g_fixture.ctx.config.frame_params = NULL;
    }

    if (g_fixture.ctx.config.wchunk != NULL)
    {
        free(g_fixture.ctx.config.wchunk);
        g_fixture.ctx.config.wchunk = NULL;
    }

    if (g_fixture.ctx.config.chunksize != NULL)
    {
        free(g_fixture.ctx.config.chunksize);
        g_fixture.ctx.config.chunksize = NULL;
    }
}
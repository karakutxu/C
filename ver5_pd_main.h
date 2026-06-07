#ifndef PRODRV_MAIN_H
#define PRODRV_MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <poll.h>

#include "msg_handler.h"
#include "sub_system.h"

typedef struct
{
    char proc_name[PROC_NAME_BUF_LEN];

    uint8_t hcs_index;

    int server_fd;

    int pme_fd;
    int slotmgr_fd;
    int csh_evt_fd;
    int csh_proc_rx_fd;

    struct pollfd pollfds[N_POLL_FDS];
    int n_pollfds;

    ProdrvConfigParam_t config;
    MsgCommParam_t comm_param;

    bool init_received;
    bool comm_param_received;

} ProdrvContext_t;

int prodrv_main(uint8_t hcs_index);

#endif
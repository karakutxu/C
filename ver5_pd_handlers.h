#ifndef PRODRV_HANDLERS_H
#define PRODRV_HANDLERS_H

#include "prodrv_main.h"

void handle_msg_init_prodrv(
        ProdrvContext_t *ctx,
        uint8_t *buf);

void handle_msg_comm_param(
        ProdrvContext_t *ctx,
        uint8_t *buf);

void handle_msg_prepare_frame(
        ProdrvContext_t *ctx,
        uint8_t *buf);

void handle_msg_tx_stats(
        ProdrvContext_t *ctx,
        uint8_t *buf);

void handle_msg_rx_stats(
        ProdrvContext_t *ctx,
        uint8_t *buf);

void handle_msg_demod_data(
        ProdrvContext_t *ctx,
        uint8_t *buf);

#endif
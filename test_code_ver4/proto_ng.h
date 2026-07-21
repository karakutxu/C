#ifndef PROTO_NG_H
#define PROTO_NG_H

#include <stdint.h>
#include "ifnet.h"
#include "proto_main.h"

void send_error_msg_pkt(struct ifnet *ifp, uint16_t err, proto_context_t *ctx);

#endif /* PROTO_NG_H */
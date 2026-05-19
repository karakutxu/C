/*
 * proto_main_loop.c
 *
 * Linux userspace event-driven main loop for:
 *
 *   - PROTOMAN control plane
 *   - slot scheduler
 *   - CSH RX/TX
 *   - Linux TUN ingress
 *   - TX frame construction
 *   - RX frame deconstruction
 *
 * Compile:
 *
 *   gcc -Wall -Wextra -O2 \
 *       proto_main_loop.c \
 *       -o proto_main_loop
 *
 * This is a COMPILE-READY architectural skeleton.
 * You still need to connect:
 *
 *   - real socket creation
 *   - real protocol structures
 *   - real make_L2frame()
 *   - real read_VMEchunk()
 *   - real proto_output()
 *   - real TUN/TAP interface
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>

/* ========================================================= */
/* CONFIG                                                     */
/* ========================================================= */

#define MAX_RX_CHUNK_SIZE 2048
#define MAX_TX_CHUNK_SIZE 2048

/* ========================================================= */
/* PLACEHOLDER TYPES                                          */
/* ========================================================= */

struct ifnet
{
    void *p;
};

struct mbuf
{
    uint8_t *data;
    uint32_t len;
};

struct sockaddr
{
    uint16_t sa_family;
    char sa_data[14];
};

struct rtentry
{
    int dummy;
};

struct RX_LAYER_2_CHUNK
{
    uint32_t len;
    uint8_t data[MAX_RX_CHUNK_SIZE];
};

struct TX_LAYER_2_CHUNK
{
    uint32_t len;
    uint8_t data[MAX_TX_CHUNK_SIZE];
};

struct protoman_msg
{
    uint32_t opcode;
    uint32_t arg;
};

struct slot_event
{
    uint32_t slot_number;
    uint32_t chunk_count;
    uint32_t first_chunk_size;
    uint32_t remaining_chunk_size;
};

/* ========================================================= */
/* GLOBAL SOCKETS                                             */
/* ========================================================= */

static int protoman_sock = -1;
static int csh_rx_sock   = -1;
static int slotmgr_sock  = -1;
static int tun_fd        = -1;

static bool proto_running = true;

/* ========================================================= */
/* PLACEHOLDER PROTOTYPES                                     */
/* ========================================================= */

int proto_output(struct ifnet *ifp,
                 struct mbuf *m0,
                 struct sockaddr *dst,
                 struct rtentry *rt0);

int make_L2frame(struct ifnet *ifp,
                 struct TX_LAYER_2_CHUNK *chunk,
                 uint32_t chunk_count,
                 uint32_t first_chunk_size,
                 uint32_t remaining_chunk_size);

void read_VMEchunk(struct ifnet *ifp,
                   struct RX_LAYER_2_CHUNK *chunk);

int write_chunk2CSH(struct ifnet *ifp,
                    struct TX_LAYER_2_CHUNK *chunk);

void handle_protoman_msg(struct ifnet *ifp,
                         struct protoman_msg *msg);

void proto_timer_tick(struct ifnet *ifp);

void replay_timer_tick(struct ifnet *ifp);

void hc_aging_tick(struct ifnet *ifp);

/* ========================================================= */
/* STUB IMPLEMENTATIONS                                       */
/* ========================================================= */

int proto_output(struct ifnet *ifp,
                 struct mbuf *m0,
                 struct sockaddr *dst,
                 struct rtentry *rt0)
{
    (void)ifp;
    (void)m0;
    (void)dst;
    (void)rt0;

    printf("proto_output()\n");

    return 0;
}

int make_L2frame(struct ifnet *ifp,
                 struct TX_LAYER_2_CHUNK *chunk,
                 uint32_t chunk_count,
                 uint32_t first_chunk_size,
                 uint32_t remaining_chunk_size)
{
    (void)ifp;

    printf("make_L2frame()\n");

    chunk->len = 128;

    memset(chunk->data,
           0xAB,
           chunk->len);

    printf("  chunk_count          = %u\n", chunk_count);
    printf("  first_chunk_size     = %u\n", first_chunk_size);
    printf("  remaining_chunk_size = %u\n", remaining_chunk_size);

    return 0;
}

void read_VMEchunk(struct ifnet *ifp,
                   struct RX_LAYER_2_CHUNK *chunk)
{
    (void)ifp;

    printf("read_VMEchunk()\n");

    printf("  RX chunk len = %u\n", chunk->len);
}

int write_chunk2CSH(struct ifnet *ifp,
                    struct TX_LAYER_2_CHUNK *chunk)
{
    (void)ifp;

    printf("write_chunk2CSH()\n");

    printf("  TX chunk len = %u\n", chunk->len);

    return 0;
}

void handle_protoman_msg(struct ifnet *ifp,
                         struct protoman_msg *msg)
{
    (void)ifp;

    printf("handle_protoman_msg()\n");

    printf("  opcode = %u\n", msg->opcode);
}

void proto_timer_tick(struct ifnet *ifp)
{
    (void)ifp;
}

void replay_timer_tick(struct ifnet *ifp)
{
    (void)ifp;
}

void hc_aging_tick(struct ifnet *ifp)
{
    (void)ifp;
}

/* ========================================================= */
/* HELPERS                                                    */
/* ========================================================= */

static int max4(int a, int b, int c, int d)
{
    int m = a;

    if (b > m) m = b;
    if (c > m) m = c;
    if (d > m) m = d;

    return m;
}

/* ========================================================= */
/* EVENT LOOP                                                 */
/* ========================================================= */

void proto_main_loop(struct ifnet *ifp)
{
    int ret;

    fd_set readfds;

    while (proto_running)
    {
        FD_ZERO(&readfds);

        /*
         * Register descriptors to monitor
         */

        if (protoman_sock >= 0)
            FD_SET(protoman_sock, &readfds);

        if (csh_rx_sock >= 0)
            FD_SET(csh_rx_sock, &readfds);

        if (slotmgr_sock >= 0)
            FD_SET(slotmgr_sock, &readfds);

        if (tun_fd >= 0)
            FD_SET(tun_fd, &readfds);

        int maxfd =
            max4(protoman_sock,
                 csh_rx_sock,
                 slotmgr_sock,
                 tun_fd);

        /*
         * Wait until at least one descriptor is readable
         */

        ret = select(maxfd + 1,
                     &readfds,
                     NULL,
                     NULL,
                     NULL);

        if (ret < 0)
        {
            perror("select");
            continue;
        }

        /* ================================================= */
        /* PHASE 1 - PROTOMAN CONTROL                        */
        /* ================================================= */

        while (protoman_sock >= 0 &&
               FD_ISSET(protoman_sock, &readfds))
        {
            struct protoman_msg msg;

            ret = recv(protoman_sock,
                       &msg,
                       sizeof(msg),
                       MSG_DONTWAIT);

            if (ret <= 0)
                break;

            handle_protoman_msg(ifp, &msg);
        }

        /* ================================================= */
        /* PHASE 2 - SLOT MANAGER EVENTS                     */
        /* ================================================= */

        while (slotmgr_sock >= 0 &&
               FD_ISSET(slotmgr_sock, &readfds))
        {
            struct slot_event ev;

            ret = recv(slotmgr_sock,
                       &ev,
                       sizeof(ev),
                       MSG_DONTWAIT);

            if (ret <= 0)
                break;

            printf("slot scheduler event\n");

            /*
             * Build TX frame
             */

            struct TX_LAYER_2_CHUNK txchunk;

            ret = make_L2frame(ifp,
                               &txchunk,
                               ev.chunk_count,
                               ev.first_chunk_size,
                               ev.remaining_chunk_size);

            if (ret == 0)
            {
                /*
                 * Send to CSH/radio layer
                 */

                write_chunk2CSH(ifp, &txchunk);
            }
        }

        /* ================================================= */
        /* PHASE 3 - RX CHUNKS FROM CSH                      */
        /* ================================================= */

        while (csh_rx_sock >= 0 &&
               FD_ISSET(csh_rx_sock, &readfds))
        {
            struct RX_LAYER_2_CHUNK rxchunk;

            ret = recv(csh_rx_sock,
                       &rxchunk,
                       sizeof(rxchunk),
                       MSG_DONTWAIT);

            if (ret <= 0)
                break;

            /*
             * Reconstruct packets from frame
             */

            read_VMEchunk(ifp, &rxchunk);
        }

        /* ================================================= */
        /* PHASE 4 - INGRESS FROM LINUX IP STACK             */
        /* ================================================= */

        while (tun_fd >= 0 &&
               FD_ISSET(tun_fd, &readfds))
        {
            uint8_t buffer[2048];

            ret = read(tun_fd,
                       buffer,
                       sizeof(buffer));

            if (ret <= 0)
                break;

            struct mbuf m;

            m.data = buffer;
            m.len  = (uint32_t)ret;

            proto_output(ifp,
                         &m,
                         NULL,
                         NULL);
        }

        /* ================================================= */
        /* PHASE 5 - PERIODIC HOUSEKEEPING                   */
        /* ================================================= */

        proto_timer_tick(ifp);

        replay_timer_tick(ifp);

        hc_aging_tick(ifp);
    }
}

/* ========================================================= */
/* MAIN                                                       */
/* ========================================================= */

int main(void)
{
    struct ifnet ifp;

    memset(&ifp, 0, sizeof(ifp));

    printf("Starting protocol main loop\n");

    /*
     * Real implementation should:
     *
     *   protoman_sock = socket(...);
     *   csh_rx_sock   = socket(...);
     *   slotmgr_sock  = socket(...);
     *   tun_fd        = open("/dev/net/tun", ...);
     */

    proto_main_loop(&ifp);

    return 0;
}
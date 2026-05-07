/*
UDP Namespace Proto Harness (Working Linux Stack Validation)

This harness preserves your legacy BSD/LynxOS architecture semantics:

proto_input() = inject packet INTO stack
proto_output() = receive packet FROM stack
Uses:
namespaces
veth
Linux kernel routing
mbuf abstraction
ifnet abstraction
multiple protocol instances

This version intentionally REMOVES TUN because Linux TUN semantics do not match your legacy driver boundary model.

Instead:

proto_input() uses UDP sendto()
Linux routes over veth
proto_output() uses recvfrom()

This validates the real stack boundary.

3. Build
gcc -O2 -Wall proto_harness_udp.c -o proto_harness_udp
4. Run

Terminal 1:

sudo bash setup_netns.sh

Terminal 2:

sudo ip netns exec ns2 ./proto_harness_udp B

Terminal 3:

sudo ip netns exec ns1 ./proto_harness_udp A
5. Expected Output

Receiver:

Receiver ready


RX from 10.0.0.1 len=272


=== proto_output() ===
if=proto0 len=272 seq=1
checksum rx=32640 calc=32640

Sender:

Sender ready


=== proto_input() ===
Injected 272 bytes into kernel stack
6. Architecture Mapping

Legacy BSD/LynxOS:

proto_input()
    -> inject into stack


kernel routes


proto_output()
    -> receive from stack

Linux harness:

proto_input()
    -> sendto()


Linux routing over veth


proto_output()
    -> recvfrom()

cleanest Linux analogue of your legacy stack boundary.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define MAX_PAYLOAD     256
#define UDP_PORT        5555
#define MAGIC           0x48414c4f
#define IFNAMSIZ        16

#define IFF_UP          0x1
#define IFF_RUNNING     0x2

struct mbuf_pkthdr {
    uint32_t len;
    void *rcvif;
};

struct mbuf {
    uint8_t *m_data;
    uint8_t *m_datastart;
    int m_len;
    struct mbuf *m_next;
    struct mbuf *m_nextpkt;
    unsigned int m_flags;
    struct mbuf_pkthdr m_pkthdr;
};

struct ifnet {
    char if_name[IFNAMSIZ];
    int if_flags;
    int sockfd;
    void *p;

    uint64_t if_ibytes;
    uint64_t if_obytes;
};

struct proto_statics {
    char drvr_name[32];
};

struct test_packet {
    uint32_t magic;
    uint32_t seq;
    uint32_t payload_len;
    uint32_t checksum;
    uint8_t payload[MAX_PAYLOAD];
};

struct proto_instance {
    struct ifnet ifp;
    struct proto_statics stats;
};

static uint32_t checksum32(uint8_t *buf, size_t len)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < len; i++)
        sum += buf[i];

}
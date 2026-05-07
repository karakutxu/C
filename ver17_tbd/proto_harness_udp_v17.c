/*
# UDP Namespace Proto Harness (Working Linux Stack Validation)

This harness preserves your legacy BSD/LynxOS architecture semantics:

* `proto_input()` = inject packet INTO stack
* `proto_output()` = receive packet FROM stack
* Uses:

  * namespaces
  * veth
  * Linux kernel routing
  * mbuf abstraction
  * ifnet abstraction
  * multiple protocol instances

This version intentionally REMOVES TUN because Linux TUN semantics do not match your legacy driver boundary model.

Instead:

* `proto_input()` uses UDP sendto()
* Linux routes over veth
* `proto_output()` uses recvfrom()

This validates the real stack boundary.

---

# 1. setup_netns.sh

```bash
#!/bin/bash
set -e

ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true

ip netns add ns1
ip netns add ns2

ip link add veth0 type veth peer name veth1

ip link set veth0 netns ns1
ip link set veth1 netns ns2

ip netns exec ns1 ip addr add 10.0.0.1/24 dev veth0
ip netns exec ns2 ip addr add 10.0.0.2/24 dev veth1

ip netns exec ns1 ip link set lo up
ip netns exec ns2 ip link set lo up

ip netns exec ns1 ip link set veth0 up
ip netns exec ns2 ip link set veth1 up

ip netns exec ns1 sysctl -w net.ipv4.ip_forward=1 >/dev/null
ip netns exec ns2 sysctl -w net.ipv4.ip_forward=1 >/dev/null

# static routes

ip netns exec ns1 ip route add 10.0.0.2 dev veth0
ip netns exec ns2 ip route add 10.0.0.1 dev veth1

echo "Namespaces ready"
```

---

# 2. proto_harness_udp.c

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

    return sum;
}

static struct mbuf *mbuf_alloc(size_t len)
{
    struct mbuf *m;

    m = calloc(1, sizeof(*m));

    if (!m)
        return NULL;

    m->m_datastart = calloc(1, len);

    if (!m->m_datastart) {
        free(m);
        return NULL;
    }

    m->m_data = m->m_datastart;
    m->m_len = len;
    m->m_pkthdr.len = len;

    return m;
}

static void mbuf_free(struct mbuf *m)
{
    if (!m)
        return;

    free(m->m_datastart);
    free(m);
}

static void build_test_packet(struct test_packet *pkt,
                              uint32_t seq)
{
    memset(pkt, 0, sizeof(*pkt));

    pkt->magic = MAGIC;
    pkt->seq = seq;
    pkt->payload_len = MAX_PAYLOAD;

    for (uint32_t i = 0; i < MAX_PAYLOAD; i++)
        pkt->payload[i] = (uint8_t)((seq + i) & 0xff);

    pkt->checksum = checksum32(pkt->payload,
                               pkt->payload_len);
}

static int validate_packet(struct test_packet *pkt)
{
    uint32_t calc;

    if (pkt->magic != MAGIC) {
        printf("BAD MAGIC 0x%x\n", pkt->magic);
        return -1;
    }

    calc = checksum32(pkt->payload,
                      pkt->payload_len);

    printf("checksum rx=%u calc=%u\n",
           pkt->checksum,
           calc);

    if (calc != pkt->checksum) {
        printf("CHECKSUM FAILURE\n");
        return -1;
    }

    for (uint32_t i = 0; i < pkt->payload_len; i++) {

        uint8_t expected = (uint8_t)((pkt->seq + i) & 0xff);

        if (pkt->payload[i] != expected) {
            printf("PAYLOAD FAILURE idx=%u\n", i);
            return -1;
        }
    }

    return 0;
}

static int proto_input(struct ifnet *ifp,
                       struct mbuf *m,
                       const char *dst_ip)
{
    struct sockaddr_in dst;
    ssize_t n;

    memset(&dst, 0, sizeof(dst));

    dst.sin_family = AF_INET;
    dst.sin_port = htons(UDP_PORT);

    if (inet_pton(AF_INET,
                  dst_ip,
                  &dst.sin_addr) != 1) {

        perror("inet_pton");
        return -1;
    }

    n = sendto(ifp->sockfd,
               m->m_data,
               m->m_len,
               0,
               (struct sockaddr *)&dst,
               sizeof(dst));

    if (n < 0) {
        perror("sendto");
        return -1;
    }

    ifp->if_obytes += n;

    printf("\n=== proto_input() ===\n");
    printf("Injected %ld bytes into kernel stack\n", n);

    return 0;
}

static void proto_output(struct ifnet *ifp,
                         struct mbuf *m)
{
    struct test_packet *pkt;

    pkt = (struct test_packet *)m->m_data;

    printf("\n=== proto_output() ===\n");
    printf("if=%s len=%d seq=%u\n",
           ifp->if_name,
           m->m_len,
           pkt->seq);

    validate_packet(pkt);

    ifp->if_ibytes += m->m_len;

    mbuf_free(m);
}

static void sender_loop(struct proto_instance *pi,
                        const char *dst_ip)
{
    uint32_t seq = 1;

    while (1) {

        struct mbuf *m;

        struct test_packet *pkt;

        m = mbuf_alloc(sizeof(*pkt));

        if (!m) {
            printf("mbuf alloc failed\n");
            exit(1);
        }

        pkt = (struct test_packet *)m->m_data;

        build_test_packet(pkt, seq++);

        proto_input(&pi->ifp,
                    m,
                    dst_ip);

        mbuf_free(m);

        sleep(1);
    }
}

static void receiver_loop(struct proto_instance *pi)
{
    while (1) {

        uint8_t buffer[2048];

        ssize_t n;

        struct mbuf *m;

        struct sockaddr_in src;

        socklen_t slen = sizeof(src);

        n = recvfrom(pi->ifp.sockfd,
                     buffer,
                     sizeof(buffer),
                     0,
                     (struct sockaddr *)&src,
                     &slen);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        printf("\nRX from %s len=%ld\n",
               inet_ntoa(src.sin_addr),
               n);

        if ((size_t)n < sizeof(struct test_packet)) {
            printf("DROP short packet\n");
            continue;
        }

        m = mbuf_alloc(n);

        if (!m) {
            printf("mbuf alloc failed\n");
            continue;
        }

        memcpy(m->m_data,
               buffer,
               n);

        proto_output(&pi->ifp, m);
    }
}

static int create_udp_socket(const char *bind_ip)
{
    int fd;

    struct sockaddr_in addr;

    fd = socket(AF_INET,
                SOCK_DGRAM,
                0);

    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);

    if (inet_pton(AF_INET,
                  bind_ip,
                  &addr.sin_addr) != 1) {

        perror("inet_pton");
        exit(1);
    }

    if (bind(fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {

        perror("bind");
        exit(1);
    }

    return fd;
}

int main(int argc, char **argv)
{
    struct proto_instance pi;

    memset(&pi, 0, sizeof(pi));

    if (argc < 2) {
        printf("usage:\n");
        printf("sender:   %s A\n", argv[0]);
        printf("receiver: %s B\n", argv[0]);
        return 1;
    }

    strcpy(pi.stats.drvr_name, "PROTO");
    strcpy(pi.ifp.if_name, "proto0");

    pi.ifp.if_flags = IFF_UP | IFF_RUNNING;

    if (strcmp(argv[1], "A") == 0) {

        pi.ifp.sockfd = create_udp_socket("10.0.0.1");

        printf("Sender ready\n");

        sender_loop(&pi,
                    "10.0.0.2");
    }
    else {

        pi.ifp.sockfd = create_udp_socket("10.0.0.2");

        printf("Receiver ready\n");

        receiver_loop(&pi);
    }

    return 0;
}
/*

---

# 3. Build

```bash
gcc -O2 -Wall proto_harness_udp.c -o proto_harness_udp
```

---

# 4. Run

Terminal 1:

```bash
sudo bash setup_netns.sh
```

Terminal 2:

```bash
sudo ip netns exec ns2 ./proto_harness_udp B
```

Terminal 3:

```bash
sudo ip netns exec ns1 ./proto_harness_udp A
```

---

# 5. Expected Output

Receiver:

```text
Receiver ready

RX from 10.0.0.1 len=272

=== proto_output() ===
if=proto0 len=272 seq=1
checksum rx=32640 calc=32640
```

Sender:

```text
Sender ready

=== proto_input() ===
Injected 272 bytes into kernel stack
```

---

# 6. Architecture Mapping

Legacy BSD/LynxOS:

```text
proto_input()
    -> inject into stack

kernel routes

proto_output()
    -> receive from stack
```

Linux harness:

```text
proto_input()
    -> sendto()

Linux routing over veth

proto_output()
    -> recvfrom()
```

This is the cleanest Linux analogue of your legacy stack boundary.

---

# 7. Next Step

Once this works, you can directly layer:

* flow table
* subflows
* priority queues
* enqueue/dequeue
* QoS scheduling
* queue drop policy

on top of:
*/

```text
proto_input()
proto_output()
```

without changing the transport architecture.

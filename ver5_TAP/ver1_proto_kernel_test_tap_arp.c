#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>

#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <linux/if_tun.h>
#include <linux/if_packet.h>

/* ============================================================
 * Configuration
 * ============================================================ */

#define TAP_A           "tap0"
#define TAP_B           "tap1"

#define VETH_A          "veth0"
#define VETH_B          "veth1"

#define UDP_PORT        5000

#define TEST_PAYLOAD    "TEST"

/*
 * Fixed demonstration MAC addresses.
 *
 * The COMSYS L2 address is deliberately equal to the
 * final octet of the MAC address.
 *
 *     MAC                       COMSYS L2
 *
 *     02:00:00:00:00:01   ->       1
 *     02:00:00:00:00:02   ->       2
 */
static const unsigned char MAC_A[ETH_ALEN] =
    { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };

static const unsigned char MAC_B[ETH_ALEN] =
    { 0x02, 0x00, 0x00, 0x00, 0x00, 0x02 };

/* ============================================================
 * Minimal mbuf replacement
 * ============================================================ */

struct mbuf {
    int len;
    unsigned char data[2048];
};

/* ============================================================
 * ifnet replacement
 *
 * Deliberately kept close to the LynxOS-style architecture.
 * ============================================================ */

struct ifnet {
    int tap_fd;
    int raw_fd;

    char name[IFNAMSIZ];
    char veth_name[IFNAMSIZ];
};

/* ============================================================
 * protocol instance
 * ============================================================ */

struct proto_instance {
    struct ifnet ifp;
    char name[32];

    unsigned char mac[ETH_ALEN];

    const char *local_ip;
    const char *remote_ip;

    const char *local_comsys_name;
};

/* ============================================================
 * Utility
 * ============================================================ */

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}


/* ============================================================
 * Print MAC
 * ============================================================ */

static void print_mac(const unsigned char *mac)
{
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0],
           mac[1],
           mac[2],
           mac[3],
           mac[4],
           mac[5]);
}


/* ============================================================
 * COMSYS L2 mapping
 *
 * DEMO ASSUMPTION:
 *
 *     MAC = xx:xx:xx:xx:xx:L2
 *
 * Therefore:
 *
 *     COMSYS L2 address = MAC[5]
 *
 * This deliberately models the fixed mapping required for
 * the demonstration.
 * ============================================================ */

static uint8_t mac_to_comsys_l2(const unsigned char *mac)
{
    return mac[5];
}


/* ============================================================
 * TAP allocation
 *
 * IFF_TAP means the userspace program receives complete
 * Ethernet frames rather than IP packets.
 * ============================================================ */

static int tap_alloc(const char *dev)
{
    struct ifreq ifr;

    int fd = open("/dev/net/tun", O_RDWR);

    if (fd < 0)
        die("open /dev/net/tun");

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0)
        die("TUNSETIFF");

    printf("Created TAP interface %s\n", ifr.ifr_name);

    return fd;
}


/* ============================================================
 * Set TAP MAC address
 * ============================================================ */

static void set_mac(const char *dev,
                    const unsigned char *mac)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
        die("socket");

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);

    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

    memcpy(ifr.ifr_hwaddr.sa_data,
           mac,
           ETH_ALEN);

    if (ioctl(fd, SIOCSIFHWADDR, &ifr) < 0)
        die("SIOCSIFHWADDR");

    close(fd);
}


/* ============================================================
 * Raw Ethernet socket on veth
 *
 * This is the other side of the demo:
 *
 * TAP <-> proto <-> raw veth <-> veth pair
 *
 * The raw socket lets proto explicitly transport the complete
 * Ethernet frame to the other namespace.
 * ============================================================ */

static int raw_socket_create(const char *ifname)
{
    int fd;

    struct sockaddr_ll sll;

    int ifindex = if_nametoindex(ifname);

    if (ifindex == 0)
        die("if_nametoindex");

    fd = socket(AF_PACKET,
                SOCK_RAW,
                htons(ETH_P_ALL));

    if (fd < 0)
        die("socket(AF_PACKET)");

    memset(&sll, 0, sizeof(sll));

    sll.sll_family   = AF_PACKET;
    sll.sll_ifindex  = ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);

    if (bind(fd,
             (struct sockaddr *)&sll,
             sizeof(sll)) < 0)
        die("bind(AF_PACKET)");

    return fd;
}


/* ============================================================
 * proto_output
 *
 * This is the important function for the demonstration.
 *
 * Linux has already:
 *
 *     - selected the route
 *     - resolved the next-hop neighbour
 *     - constructed the Ethernet header
 *
 * Therefore the destination MAC in m->data is the MAC selected
 * by Linux.
 *
 * This is the Linux equivalent of the legacy:
 *
 *     dst->sa_data[5]
 * ============================================================ */

static int proto_output(struct proto_instance *inst,
                        struct mbuf *m)
{
    struct ethhdr *eth;

    if (m->len < (int)sizeof(struct ethhdr))
        return -1;

    eth = (struct ethhdr *)m->data;

    printf("\n[%s] proto_output\n",
           inst->name);

    printf("    Ethernet frame length = %d\n",
           m->len);

    printf("    Destination MAC       = ");
    print_mac(eth->h_dest);
    printf("\n");

    printf("    Source MAC            = ");
    print_mac(eth->h_source);
    printf("\n");

    /*
     * Only demonstrate the COMSYS mapping for a unicast
     * destination.
     *
     * ARP requests are normally broadcast:
     *
     *     ff:ff:ff:ff:ff:ff
     *
     * so they are not a COMSYS L2 destination.
     */

    if (memcmp(eth->h_dest,
               "\xff\xff\xff\xff\xff\xff",
               ETH_ALEN) != 0)
    {
        uint8_t l2 = mac_to_comsys_l2(eth->h_dest);

        printf("    COMSYS L2 address     = %u\n",
               l2);
    }
    else
    {
        printf("    Destination           = broadcast\n");
    }

    /*
     * Forward the complete Ethernet frame over the veth.
     */

    int rc = send(inst->ifp.raw_fd,
                  m->data,
                  m->len,
                  0);

    if (rc < 0)
    {
        perror("send raw Ethernet frame");
        return -1;
    }

    if (rc != m->len)
    {
        fprintf(stderr,
                "[%s] Short Ethernet send: %d/%d\n",
                inst->name,
                rc,
                m->len);

        return -1;
    }

    return 0;
}


/* ============================================================
 * proto_input
 *
 * Frame has arrived from the remote proto instance.
 *
 * The frame is written into the local TAP interface so that
 * Linux sees it as an Ethernet frame arriving on the TAP NIC.
 * ============================================================ */

static int proto_input(struct proto_instance *inst,
                       struct mbuf *m)
{
    struct ethhdr *eth;

    if (m->len < (int)sizeof(struct ethhdr))
        return -1;

    eth = (struct ethhdr *)m->data;

    printf("\n[%s] proto_input\n",
           inst->name);

    printf("    Received Ethernet frame: %d bytes\n",
           m->len);

    printf("    Destination MAC         = ");
    print_mac(eth->h_dest);
    printf("\n");

    printf("    Source MAC              = ");
    print_mac(eth->h_source);
    printf("\n");

    /*
     * Inject complete Ethernet frame into the Linux kernel
     * through TAP.
     */

    int rc = write(inst->ifp.tap_fd,
                   m->data,
                   m->len);

    if (rc < 0)
    {
        perror("write TAP");
        return -1;
    }

    if (rc != m->len)
    {
        fprintf(stderr,
                "[%s] Short TAP write: %d/%d\n",
                inst->name,
                rc,
                m->len);

        return -1;
    }

    return 0;
}


/* ============================================================
 * Display IPv4 information
 * ============================================================ */

static void display_ipv4(struct proto_instance *inst,
                         struct mbuf *m)
{
    struct ethhdr *eth;
    struct iphdr *ip;

    if (m->len <
        (int)(sizeof(struct ethhdr) +
              sizeof(struct iphdr)))
        return;

    eth = (struct ethhdr *)m->data;

    if (ntohs(eth->h_proto) != ETH_P_IP)
        return;

    ip = (struct iphdr *)(m->data +
                          sizeof(struct ethhdr));

    struct in_addr src;
    struct in_addr dst;

    src.s_addr = ip->saddr;
    dst.s_addr = ip->daddr;

    printf("    IPv4 source             = %s\n",
           inet_ntoa(src));

    printf("    IPv4 destination        = %s\n",
           inet_ntoa(dst));
}


/* ============================================================
 * Process one TAP frame
 *
 * This corresponds conceptually to proto_output() receiving
 * an Ethernet frame from the Linux networking stack.
 * ============================================================ */

static void process_tap_frame(struct proto_instance *inst)
{
    struct mbuf m;

    int n = read(inst->ifp.tap_fd,
                 m.data,
                 sizeof(m.data));

    if (n < 0)
    {
        if (errno == EINTR)
            return;

        die("read TAP");
    }

    if (n == 0)
        return;

    m.len = n;

    display_ipv4(inst, &m);

    proto_output(inst, &m);
}


/* ============================================================
 * Process frame arriving from remote veth
 *
 * The remote proto instance transmitted this complete Ethernet
 * frame through its veth.
 * ============================================================ */

static void process_raw_frame(struct proto_instance *inst)
{
    struct mbuf m;

    int n = recv(inst->ifp.raw_fd,
                 m.data,
                 sizeof(m.data),
                 0);

    if (n < 0)
    {
        if (errno == EINTR)
            return;

        die("recv raw");
    }

    if (n == 0)
        return;

    m.len = n;

    proto_input(inst, &m);
}


/* ============================================================
 * Configure TAP interface using normal Linux commands.
 *
 * This is intentionally kept outside the protocol code.
 * ============================================================ */

static void configure_interface_A(void)
{
    system("ip link set tap0 up");

    system("ip addr add 192.168.0.1/24 dev tap0");

    system("ip addr add 10.0.0.99/24 dev tap0");

    /*
     * Destination network is reached through the Linux
     * next-hop neighbour 192.168.0.2.
     */

    system("ip route add 10.0.1.0/24 via 192.168.0.2 dev tap0");
}


static void configure_interface_B(void)
{
    system("ip link set tap1 up");

    system("ip addr add 192.168.0.2/24 dev tap1");

    system("ip addr add 10.0.1.99/24 dev tap1");

    /*
     * Return route.
     */

    system("ip route add 10.0.0.0/24 via 192.168.0.1 dev tap1");
}


/* ============================================================
 * UDP test application
 *
 * The application uses the normal Linux IP stack.
 *
 * proto does NOT construct this packet.
 *
 * Linux constructs the Ethernet frame and resolves the
 * neighbour before the frame appears on TAP.
 * ============================================================ */

static void send_test_packet(void)
{
    int fd;

    struct sockaddr_in local;
    struct sockaddr_in remote;

    fd = socket(AF_INET,
                SOCK_DGRAM,
                0);

    if (fd < 0)
        die("UDP socket");

    memset(&local, 0, sizeof(local));

    local.sin_family = AF_INET;
    local.sin_port = htons(4000);

    inet_pton(AF_INET,
              "10.0.0.99",
              &local.sin_addr);

    if (bind(fd,
             (struct sockaddr *)&local,
             sizeof(local)) < 0)
        die("UDP bind");

    memset(&remote, 0, sizeof(remote));

    remote.sin_family = AF_INET;
    remote.sin_port = htons(UDP_PORT);

    inet_pton(AF_INET,
              "10.0.1.99",
              &remote.sin_addr);

    printf("\n[A] Sending UDP TEST packet\n");

    if (sendto(fd,
               TEST_PAYLOAD,
               strlen(TEST_PAYLOAD),
               0,
               (struct sockaddr *)&remote,
               sizeof(remote)) < 0)
    {
        die("sendto");
    }

    close(fd);
}


/* ============================================================
 * UDP receiver
 *
 * This is simply an application sitting behind the Linux
 * networking stack in ns2.
 * ============================================================ */

static void receive_test_packet(void)
{
    int fd;

    struct sockaddr_in local;

    char buffer[256];

    fd = socket(AF_INET,
                SOCK_DGRAM,
                0);

    if (fd < 0)
        die("UDP socket");

    memset(&local, 0, sizeof(local));

    local.sin_family = AF_INET;
    local.sin_port = htons(UDP_PORT);

    inet_pton(AF_INET,
              "10.0.1.99",
              &local.sin_addr);

    if (bind(fd,
             (struct sockaddr *)&local,
             sizeof(local)) < 0)
        die("UDP receiver bind");

    printf("[B] UDP receiver listening on "
           "10.0.1.99:%d\n",
           UDP_PORT);

    int n = recv(fd,
                 buffer,
                 sizeof(buffer) - 1,
                 0);

    if (n < 0)
        die("UDP recv");

    buffer[n] = '\0';

    printf("\n[B] UDP application received: \"%s\"\n",
           buffer);

    if (strcmp(buffer, TEST_PAYLOAD) == 0)
    {
        printf("[B] TEST PASSED\n");
    }
    else
    {
        printf("[B] TEST FAILED\n");
    }

    close(fd);
}


/* ============================================================
 * Main event loop
 *
 * proto is effectively doing:
 *
 *       TAP <-> veth
 *
 * while inspecting the Ethernet frame.
 * ============================================================ */

static void proto_loop(struct proto_instance *inst)
{
    fd_set readfds;

    int maxfd;

    while (1)
    {
        FD_ZERO(&readfds);

        FD_SET(inst->ifp.tap_fd,
               &readfds);

        FD_SET(inst->ifp.raw_fd,
               &readfds);

        maxfd = inst->ifp.tap_fd;

        if (inst->ifp.raw_fd > maxfd)
            maxfd = inst->ifp.raw_fd;

        int rc = select(maxfd + 1,
                        &readfds,
                        NULL,
                        NULL,
                        NULL);

        if (rc < 0)
        {
            if (errno == EINTR)
                continue;

            die("select");
        }

        /*
         * Frame generated by the Linux networking stack.
         *
         * This is the important path:
         *
         *     Linux routing
         *          ↓
         *     neighbour lookup
         *          ↓
         *     Ethernet destination MAC
         *          ↓
         *     TAP
         *          ↓
         *     proto_output()
         */
        if (FD_ISSET(inst->ifp.tap_fd,
                     &readfds))
        {
            process_tap_frame(inst);
        }

        /*
         * Frame arriving from remote proto instance.
         */
        if (FD_ISSET(inst->ifp.raw_fd,
                     &readfds))
        {
            process_raw_frame(inst);
        }
    }
}


/* ============================================================
 * MAIN
 * ============================================================ */

int main(int argc, char *argv[])
{
    struct proto_instance inst;

    memset(&inst, 0, sizeof(inst));

    if (argc != 2)
    {
        fprintf(stderr,
                "Usage: %s A|B\n",
                argv[0]);

        return EXIT_FAILURE;
    }


    /* ========================================================
     * INSTANCE A
     * ======================================================== */

    if (argv[1][0] == 'A')
    {
        strcpy(inst.name,
               "INSTANCE_A");

        strcpy(inst.ifp.name,
               TAP_A);

        strcpy(inst.ifp.veth_name,
               VETH_A);

        memcpy(inst.mac,
               MAC_A,
               ETH_ALEN);

        inst.local_ip  = "10.0.0.99";
        inst.remote_ip = "10.0.1.99";

        /*
         * Create TAP.
         */

        inst.ifp.tap_fd =
            tap_alloc(TAP_A);

        /*
         * Assign fixed MAC.
         *
         *     02:00:00:00:00:01
         *
         * therefore COMSYS L2 = 1.
         */

        set_mac(TAP_A,
                MAC_A);

        /*
         * Configure normal Linux networking.
         */

        configure_interface_A();

        /*
         * Raw socket used only to transport the complete
         * Ethernet frame across the veth pair.
         */

        inst.ifp.raw_fd =
            raw_socket_create(VETH_A);

        printf("\n");
        printf("============================================\n");
        printf(" PROTO INSTANCE A\n");
        printf("============================================\n");

        printf("TAP       : %s\n",
               TAP_A);

        printf("TAP MAC   : ");
        print_mac(MAC_A);
        printf("\n");

        printf("COMSYS L2 : %u\n",
               mac_to_comsys_l2(MAC_A));

        printf("IP        : %s\n",
               inst.local_ip);

        printf("============================================\n");

        /*
         * Allow the interface and routing configuration to
         * settle before generating traffic.
         */

        sleep(2);

        /*
         * Generate traffic through the NORMAL Linux IP stack.
         *
         * proto itself does not construct the Ethernet header.
         */

        send_test_packet();

        /*
         * Continue processing until the demo is manually
         * terminated.
         */

        proto_loop(&inst);
    }


    /* ========================================================
     * INSTANCE B
     * ======================================================== */

    else if (argv[1][0] == 'B')
    {
        strcpy(inst.name,
               "INSTANCE_B");

        strcpy(inst.ifp.name,
               TAP_B);

        strcpy(inst.ifp.veth_name,
               VETH_B);

        memcpy(inst.mac,
               MAC_B,
               ETH_ALEN);

        inst.local_ip  = "10.0.1.99";
        inst.remote_ip = "10.0.0.99";

        /*
         * Create TAP.
         */

        inst.ifp.tap_fd =
            tap_alloc(TAP_B);

        /*
         * Fixed MAC:
         *
         *     02:00:00:00:00:02
         *
         * therefore COMSYS L2 = 2.
         */

        set_mac(TAP_B,
                MAC_B);

        /*
         * Configure normal Linux networking.
         */

        configure_interface_B();

        /*
         * Raw Ethernet transport.
         */

        inst.ifp.raw_fd =
            raw_socket_create(VETH_B);

        printf("\n");
        printf("============================================\n");
        printf(" PROTO INSTANCE B\n");
        printf("============================================\n");

        printf("TAP       : %s\n",
               TAP_B);

        printf("TAP MAC   : ");
        print_mac(MAC_B);
        printf("\n");

        printf("COMSYS L2 : %u\n",
               mac_to_comsys_l2(MAC_B));

        printf("IP        : %s\n",
               inst.local_ip);

        printf("============================================\n");

        /*
         * The UDP application receives the packet through
         * the normal Linux IP stack.
         *
         * The proto loop must run simultaneously, however,
         * so we cannot simply call receive_test_packet()
         * here.
         *
         * The simplest demonstration is therefore to run
         * the proto loop and let the UDP application be
         * observed separately.
         */

        printf("\n[B] Starting proto loop.\n");
        printf("[B] UDP application should be listening on "
               "10.0.1.99:%d\n",
               UDP_PORT);

        /*
         * For this minimal demo, proto_loop is the main
         * process. The UDP receiver can instead be started
         * with netcat/socat, or the loop can be extended
         * with a third FD.
         */

        proto_loop(&inst);
    }

    else
    {
        fprintf(stderr,
                "Argument must be A or B\n");

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
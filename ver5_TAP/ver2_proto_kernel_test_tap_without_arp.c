#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>

#include <net/if.h>
#include <net/if_arp.h>

#include <netinet/in.h>

#include <linux/if_tun.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>


/* ============================================================
 * Configuration
 * ============================================================ */

#define TAP_A       "tap0"
#define TAP_B       "tap1"

#define VETH_A      "veth0"
#define VETH_B      "veth1"

#define UDP_PORT    5000

#define TEST_PAYLOAD    "TEST"


/*
 * Fixed demonstration MAC addresses.
 *
 * The last octet is deliberately identical to the
 * demonstration COMSYS L2 address.
 *
 *     MAC                       L2
 *
 *     02:00:00:00:00:01        1
 *     02:00:00:00:00:02        2
 */

static const unsigned char MAC_A[ETH_ALEN] =
{
    0x02, 0x00, 0x00, 0x00, 0x00, 0x01
};

static const unsigned char MAC_B[ETH_ALEN] =
{
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02
};


/* ============================================================
 * Minimal mbuf replacement
 *
 * The real port can substitute the Linux compatibility mbuf.
 * ============================================================ */

struct mbuf
{
    int len;

    unsigned char data[2048];
};


/* ============================================================
 * ifnet replacement
 *
 * TAP is the interface to the Linux networking stack.
 *
 * raw_fd is the Ethernet transport used by the demo to connect
 * proto A to proto B.
 * ============================================================ */

struct ifnet
{
    int tap_fd;
    int raw_fd;

    char name[IFNAMSIZ];
    char veth_name[IFNAMSIZ];
};


/* ============================================================
 * Protocol instance
 * ============================================================ */

struct proto_instance
{
    struct ifnet ifp;

    char name[32];

    unsigned char mac[ETH_ALEN];

    const char *local_ip;
    const char *remote_ip;
};


/* ============================================================
 * Error handling
 * ============================================================ */

static void die(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}


/* ============================================================
 * Print MAC address
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
 * MAC -> COMSYS L2 mapping
 *
 * DEMO ONLY
 *
 * The requested demonstration has a fixed mapping:
 *
 *     MAC xx:xx:xx:xx:xx:N
 *
 * maps to:
 *
 *     L2 address N
 *
 * In the real system this function can be replaced by the
 * actual L2 addressing/configuration mechanism.
 * ============================================================ */

static uint8_t mac_to_l2(const unsigned char *mac)
{
    return mac[5];
}


/* ============================================================
 * Create TAP interface
 *
 * IFF_TAP means that userspace receives complete Ethernet
 * frames.
 *
 * IFF_NO_PI means there is no extra TAP packet-information
 * header.
 * ============================================================ */

static int tap_alloc(const char *dev)
{
    struct ifreq ifr;

    int fd;

    fd = open("/dev/net/tun", O_RDWR);

    if (fd < 0)
        die("open /dev/net/tun");

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    strncpy(ifr.ifr_name,
            dev,
            IFNAMSIZ - 1);

    if (ioctl(fd,
              TUNSETIFF,
              &ifr) < 0)
    {
        die("TUNSETIFF");
    }

    return fd;
}


/* ============================================================
 * Set TAP MAC address
 * ============================================================ */

static void set_interface_mac(const char *dev,
                               const unsigned char *mac)
{
    int fd;

    struct ifreq ifr;

    fd = socket(AF_INET,
                SOCK_DGRAM,
                0);

    if (fd < 0)
        die("socket");

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name,
            dev,
            IFNAMSIZ - 1);

    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

    memcpy(ifr.ifr_hwaddr.sa_data,
           mac,
           ETH_ALEN);

    if (ioctl(fd,
              SIOCSIFHWADDR,
              &ifr) < 0)
    {
        die("SIOCSIFHWADDR");
    }

    close(fd);
}


/* ============================================================
 * Create raw Ethernet socket on veth
 *
 * This is NOT the networking interface seen by the IP stack.
 *
 * It is simply the demo transport between the two proto
 * instances.
 *
 * The complete Ethernet frame obtained from TAP is sent here.
 * ============================================================ */

static int raw_socket_create(const char *ifname)
{
    int fd;

    int ifindex;

    struct sockaddr_ll sll;

    int ignore_outgoing = 1;


    ifindex = if_nametoindex(ifname);

    if (ifindex == 0)
        die("if_nametoindex");


    fd = socket(AF_PACKET,
                SOCK_RAW,
                htons(ETH_P_ALL));

    if (fd < 0)
        die("socket(AF_PACKET)");


    /*
     * Do not give proto its own transmitted packet back.
     *
     * Without this option an AF_PACKET socket can receive a
     * PACKET_OUTGOING copy of a frame that proto itself sent.
     *
     * We only want frames arriving from the remote namespace.
     */

    if (setsockopt(fd,
                   SOL_PACKET,
                   PACKET_IGNORE_OUTGOING,
                   &ignore_outgoing,
                   sizeof(ignore_outgoing)) < 0)
    {
        die("PACKET_IGNORE_OUTGOING");
    }


    memset(&sll, 0, sizeof(sll));

    sll.sll_family = AF_PACKET;

    sll.sll_ifindex = ifindex;

    sll.sll_protocol = htons(ETH_P_ALL);


    if (bind(fd,
             (struct sockaddr *)&sll,
             sizeof(sll)) < 0)
    {
        die("bind(AF_PACKET)");
    }


    return fd;
}


/* ============================================================
 * Configure INSTANCE A
 *
 * Important:
 *
 *     ARP is explicitly disabled.
 *
 * The neighbour/L2 mapping is installed statically.
 *
 * There is therefore no ARP exchange.
 * ============================================================ */

static void configure_instance_A(void)
{
    /*
     * Disable ARP on TAP.
     */

    if (system("ip link set dev tap0 arp off") != 0)
        exit(EXIT_FAILURE);


    /*
     * Bring TAP up.
     */

    if (system("ip link set dev tap0 up") != 0)
        exit(EXIT_FAILURE);


    /*
     * Management/next-hop subnet.
     */

    if (system("ip addr add 192.168.0.1/24 dev tap0") != 0)
        exit(EXIT_FAILURE);


    /*
     * Test/application subnet.
     */

    if (system("ip addr add 10.0.0.99/24 dev tap0") != 0)
        exit(EXIT_FAILURE);


    /*
     * Normal Linux IP routing.
     *
     * Traffic for 10.0.1.0/24 is sent through next-hop
     * 192.168.0.2.
     */

    if (system(
        "ip route replace "
        "10.0.1.0/24 "
        "via 192.168.0.2 "
        "dev tap0") != 0)
    {
        exit(EXIT_FAILURE);
    }


    /*
     * EXPLICIT L2 CONFIGURATION.
     *
     * No ARP is performed.
     *
     * Linux is told directly:
     *
     *     192.168.0.2
     *          ->
     *     02:00:00:00:00:02
     *
     * NUD permanent prevents neighbour discovery.
     */

    if (system(
        "ip neigh replace "
        "192.168.0.2 "
        "lladdr 02:00:00:00:00:02 "
        "nud permanent "
        "dev tap0") != 0)
    {
        exit(EXIT_FAILURE);
    }
}


/* ============================================================
 * Configure INSTANCE B
 *
 * Same arrangement in the opposite direction.
 * ============================================================ */

static void configure_instance_B(void)
{
    /*
     * Disable ARP.
     */

    if (system("ip link set dev tap1 arp off") != 0)
        exit(EXIT_FAILURE);


    /*
     * Bring TAP up.
     */

    if (system("ip link set dev tap1 up") != 0)
        exit(EXIT_FAILURE);


    /*
     * Next-hop subnet.
     */

    if (system("ip addr add 192.168.0.2/24 dev tap1") != 0)
        exit(EXIT_FAILURE);


    /*
     * Application subnet.
     */

    if (system("ip addr add 10.0.1.99/24 dev tap1") != 0)
        exit(EXIT_FAILURE);


    /*
     * Return route.
     */

    if (system(
        "ip route replace "
        "10.0.0.0/24 "
        "via 192.168.0.1 "
        "dev tap1") != 0)
    {
        exit(EXIT_FAILURE);
    }


    /*
     * EXPLICIT L2 CONFIGURATION.
     *
     *     192.168.0.1
     *          ->
     *     02:00:00:00:00:01
     */

    if (system(
        "ip neigh replace "
        "192.168.0.1 "
        "lladdr 02:00:00:00:00:01 "
        "nud permanent "
        "dev tap1") != 0)
    {
        exit(EXIT_FAILURE);
    }
}


/* ============================================================
 * Create UDP receiver
 *
 * This socket sits behind the Linux IP stack.
 *
 * The test packet eventually reaches this socket after:
 *
 *     TAP
 *       ->
 *     Linux Ethernet processing
 *       ->
 *     Linux IP processing
 *       ->
 *     UDP
 * ============================================================ */

static int udp_receiver_create(void)
{
    int fd;

    struct sockaddr_in address;


    fd = socket(AF_INET,
                SOCK_DGRAM,
                0);

    if (fd < 0)
        die("UDP socket");


    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;

    address.sin_port = htons(UDP_PORT);

    if (inet_pton(AF_INET,
                  "10.0.1.99",
                  &address.sin_addr) != 1)
    {
        fprintf(stderr,
                "inet_pton failed\n");

        exit(EXIT_FAILURE);
    }


    if (bind(fd,
             (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        die("UDP bind");
    }


    return fd;
}


/* ============================================================
 * Send test packet
 *
 * IMPORTANT:
 *
 * This is an ordinary Linux UDP operation.
 *
 * The application does NOT construct:
 *
 *     Ethernet header
 *     destination MAC
 *
 * Linux does that as a result of its routing and static
 * neighbour configuration.
 * ============================================================ */

static void send_test_packet(void)
{
    int fd;

    struct sockaddr_in local;
    struct sockaddr_in destination;


    fd = socket(AF_INET,
                SOCK_DGRAM,
                0);

    if (fd < 0)
        die("UDP socket");


    memset(&local, 0, sizeof(local));

    local.sin_family = AF_INET;

    local.sin_port = htons(4000);

    if (inet_pton(AF_INET,
                  "10.0.0.99",
                  &local.sin_addr) != 1)
    {
        fprintf(stderr,
                "inet_pton failed\n");

        exit(EXIT_FAILURE);
    }


    /*
     * Force the source address to 10.0.0.99.
     */

    if (bind(fd,
             (struct sockaddr *)&local,
             sizeof(local)) < 0)
    {
        die("UDP bind");
    }


    memset(&destination, 0, sizeof(destination));

    destination.sin_family = AF_INET;

    destination.sin_port = htons(UDP_PORT);

    if (inet_pton(AF_INET,
                  "10.0.1.99",
                  &destination.sin_addr) != 1)
    {
        fprintf(stderr,
                "inet_pton failed\n");

        exit(EXIT_FAILURE);
    }


    printf("\n");
    printf("[A] Linux application sending UDP packet\n");
    printf("    source      = 10.0.0.99:4000\n");
    printf("    destination = 10.0.1.99:%d\n",
           UDP_PORT);
    printf("\n");


    if (sendto(fd,
               TEST_PAYLOAD,
               strlen(TEST_PAYLOAD),
               0,
               (struct sockaddr *)&destination,
               sizeof(destination)) < 0)
    {
        die("sendto");
    }


    close(fd);
}


/* ============================================================
 * proto_output
 *
 * THIS IS THE KEY FUNCTION IN THE DEMO.
 *
 * The packet arriving here has already been processed by the
 * Linux networking stack.
 *
 * Linux has selected:
 *
 *     destination IP
 *            ->
 *     route
 *            ->
 *     next-hop IP
 *            ->
 *     static neighbour entry
 *            ->
 *     destination MAC
 *
 * TAP gives proto the resulting COMPLETE Ethernet frame.
 *
 * Therefore:
 *
 *     eth->h_dest
 *
 * is the Layer-2 destination selected by Linux.
 *
 * This is the conceptual Linux/TAP equivalent of the legacy
 * code obtaining its L2 destination from:
 *
 *     dst->sa_data[5]
 * ============================================================ */

static int proto_output(struct proto_instance *inst,
                        struct mbuf *m)
{
    struct ethhdr *eth;


    if (m->len < (int)sizeof(struct ethhdr))
    {
        fprintf(stderr,
                "[%s] Frame too short\n",
                inst->name);

        return -1;
    }


    eth = (struct ethhdr *)m->data;


    printf("\n");
    printf("============================================\n");
    printf("[%s] proto_output()\n",
           inst->name);
    printf("============================================\n");


    printf("Ethernet frame length = %d\n",
           m->len);


    printf("Destination MAC       = ");

    print_mac(eth->h_dest);

    printf("\n");


    printf("Source MAC             = ");

    print_mac(eth->h_source);

    printf("\n");


    /*
     * The important operation.
     *
     * This is the L2 address that proto obtains from the
     * Ethernet destination MAC supplied by Linux.
     */

    uint8_t l2 = mac_to_l2(eth->h_dest);


    printf("COMSYS L2 address      = %u\n",
           l2);


    printf("--------------------------------------------\n");

    printf("Linux selected the destination MAC.\n");
    printf("proto extracted the L2 address from it.\n");

    printf("--------------------------------------------\n");


    /*
     * Forward the COMPLETE Ethernet frame through the
     * demonstration transport.
     */

    int rc = send(inst->ifp.raw_fd,
                  m->data,
                  m->len,
                  0);


    if (rc < 0)
    {
        perror("send raw Ethernet");

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
 * The complete Ethernet frame has arrived from the remote
 * protocol instance.
 *
 * proto injects it into the local Linux networking stack
 * through TAP.
 * ============================================================ */

static int proto_input(struct proto_instance *inst,
                       struct mbuf *m)
{
    struct ethhdr *eth;


    if (m->len < (int)sizeof(struct ethhdr))
    {
        fprintf(stderr,
                "[%s] Received frame too short\n",
                inst->name);

        return -1;
    }


    eth = (struct ethhdr *)m->data;


    printf("\n");
    printf("============================================\n");
    printf("[%s] proto_input()\n",
           inst->name);
    printf("============================================\n");


    printf("Ethernet frame length = %d\n",
           m->len);


    printf("Destination MAC       = ");

    print_mac(eth->h_dest);

    printf("\n");


    printf("Source MAC             = ");

    print_mac(eth->h_source);

    printf("\n");


    /*
     * Inject the COMPLETE Ethernet frame into the local
     * Linux networking stack.
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
 * Process frame received from TAP
 *
 * TAP -> proto
 *
 * This is the path used for packets generated by the Linux
 * networking stack.
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


    /*
     * Linux has supplied a complete Ethernet frame.
     */

    proto_output(inst,
                 &m);
}


/* ============================================================
 * Process frame arriving through the veth
 *
 * remote proto
 *      ->
 * veth
 *      ->
 * local raw Ethernet socket
 *      ->
 * proto_input()
 *      ->
 * TAP
 * ============================================================ */

static bool process_raw_frame(struct proto_instance *inst)
{
    struct mbuf m;


    int n = recv(inst->ifp.raw_fd,
                 m.data,
                 sizeof(m.data),
                 0);


    if (n < 0)
    {
        if (errno == EINTR)
            return false;

        die("recv raw");
    }


    if (n == 0)
        return false;


    m.len = n;


    proto_input(inst,
                &m);


    return true;
}


/* ============================================================
 * INSTANCE B event loop
 *
 * Three possible sources:
 *
 *     raw Ethernet
 *         |
 *         v
 *       proto
 *         |
 *         v
 *       TAP
 *         |
 *         v
 *    Linux IP stack
 *
 * and:
 *
 *     UDP socket
 *
 * The latter lets us prove that the packet has successfully
 * traversed the Linux IP stack.
 * ============================================================ */

static void run_instance_B(struct proto_instance *inst)
{
    int udp_fd;


    udp_fd = udp_receiver_create();


    printf("\n");
    printf("============================================\n");
    printf(" INSTANCE B\n");
    printf("============================================\n");

    printf("TAP interface       : %s\n",
           inst->ifp.name);

    printf("TAP MAC             : ");

    print_mac(inst->mac);

    printf("\n");

    printf("COMSYS L2 address   : %u\n",
           mac_to_l2(inst->mac));

    printf("IP address          : %s\n",
           inst->local_ip);

    printf("ARP                 : DISABLED\n");

    printf("UDP listener        : 10.0.1.99:%d\n",
           UDP_PORT);

    printf("============================================\n");
    printf("\n");


    for (;;)
    {
        fd_set readfds;

        int max_fd;


        FD_ZERO(&readfds);


        FD_SET(inst->ifp.raw_fd,
               &readfds);

        FD_SET(inst->ifp.tap_fd,
               &readfds);

        FD_SET(udp_fd,
               &readfds);


        max_fd = inst->ifp.raw_fd;


        if (inst->ifp.tap_fd > max_fd)
            max_fd = inst->ifp.tap_fd;


        if (udp_fd > max_fd)
            max_fd = udp_fd;


        int rc = select(max_fd + 1,
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
         * Frame arriving from remote proto.
         *
         * This is the main RX path.
         */

        if (FD_ISSET(inst->ifp.raw_fd,
                     &readfds))
        {
            process_raw_frame(inst);
        }


        /*
         * A frame generated by the local Linux IP stack.
         *
         * Normally this is not needed for this one-way test,
         * but it keeps the protocol structure symmetric.
         */

        if (FD_ISSET(inst->ifp.tap_fd,
                     &readfds))
        {
            process_tap_frame(inst);
        }


        /*
         * Packet has reached the UDP application.
         */

        if (FD_ISSET(udp_fd,
                     &readfds))
        {
            char buffer[256];

            int n = recv(udp_fd,
                         buffer,
                         sizeof(buffer) - 1,
                         0);


            if (n < 0)
                die("UDP recv");


            buffer[n] = '\0';


            printf("\n");
            printf("============================================\n");
            printf("[B] UDP application received packet\n");
            printf("============================================\n");

            printf("Payload = \"%s\"\n",
                   buffer);


            if (strcmp(buffer,
                       TEST_PAYLOAD) == 0)
            {
                printf("\n");
                printf("[B] TEST PASSED\n");
            }
            else
            {
                printf("\n");
                printf("[B] TEST FAILED\n");
            }


            printf("\n");

            close(udp_fd);

            return;
        }
    }
}


/* ============================================================
 * INSTANCE A event loop
 *
 * First generate the test packet.
 *
 * The packet enters the Linux IP stack.
 *
 * It then appears on TAP after Linux has selected the
 * destination MAC from the static neighbour entry.
 * ============================================================ */

static void run_instance_A(struct proto_instance *inst)
{
    /*
     * Give the receiver time to initialise.
     */

    sleep(2);


    /*
     * Generate ordinary IP traffic.
     *
     * No Ethernet header is constructed here.
     *
     * No MAC address is specified here.
     *
     * Linux determines the L2 destination.
     */

    send_test_packet();


    /*
     * Now process the resulting Ethernet frame from TAP.
     */

    for (;;)
    {
        fd_set readfds;

        int max_fd;


        FD_ZERO(&readfds);


        FD_SET(inst->ifp.tap_fd,
               &readfds);

        FD_SET(inst->ifp.raw_fd,
               &readfds);


        max_fd = inst->ifp.tap_fd;


        if (inst->ifp.raw_fd > max_fd)
            max_fd = inst->ifp.raw_fd;


        int rc = select(max_fd + 1,
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
         * Linux-generated Ethernet frame.
         */

        if (FD_ISSET(inst->ifp.tap_fd,
                     &readfds))
        {
            process_tap_frame(inst);
        }


        /*
         * Not normally required for the one-way test, but
         * retained for symmetry with instance B.
         */

        if (FD_ISSET(inst->ifp.raw_fd,
                     &readfds))
        {
            process_raw_frame(inst);
        }
    }
}


/* ============================================================
 * Main
 * ============================================================ */

int main(int argc,
         char *argv[])
{
    struct proto_instance inst;

    bool instance_A;


    if (argc != 2)
    {
        fprintf(stderr,
                "Usage: %s A|B\n",
                argv[0]);

        return EXIT_FAILURE;
    }


    if (argv[1][0] == 'A')
    {
        instance_A = true;
    }
    else if (argv[1][0] == 'B')
    {
        instance_A = false;
    }
    else
    {
        fprintf(stderr,
                "Argument must be A or B\n");

        return EXIT_FAILURE;
    }


    memset(&inst,
           0,
           sizeof(inst));


    /* --------------------------------------------------------
     * INSTANCE A
     * -------------------------------------------------------- */

    if (instance_A)
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


        inst.local_ip =
            "10.0.0.99";


        inst.remote_ip =
            "10.0.1.99";


        /*
         * Create TAP.
         */

        inst.ifp.tap_fd =
            tap_alloc(TAP_A);


        /*
         * Give TAP its explicit MAC address.
         */

        set_interface_mac(TAP_A,
                           MAC_A);


        /*
         * Configure Linux networking.
         *
         * This includes:
         *
         *     IP addresses
         *     routing
         *     ARP disabled
         *     static neighbour/L2 entry
         */

        configure_instance_A();


        /*
         * Create raw Ethernet transport.
         */

        inst.ifp.raw_fd =
            raw_socket_create(VETH_A);


        printf("\n");
        printf("============================================\n");
        printf(" INSTANCE A\n");
        printf("============================================\n");

        printf("TAP interface       : %s\n",
               inst.ifp.name);

        printf("TAP MAC             : ");

        print_mac(inst.mac);

        printf("\n");

        printf("COMSYS L2 address   : %u\n",
               mac_to_l2(inst.mac));

        printf("IP address          : %s\n",
               inst.local_ip);

        printf("ARP                 : DISABLED\n");

        printf("Static peer L2      : ");

        print_mac(MAC_B);

        printf("\n");

        printf("Peer L2 address     : %u\n",
               mac_to_l2(MAC_B));

        printf("============================================\n");


        run_instance_A(&inst);
    }


    /* --------------------------------------------------------
     * INSTANCE B
     * -------------------------------------------------------- */

    else
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


        inst.local_ip =
            "10.0.1.99";


        inst.remote_ip =
            "10.0.0.99";


        /*
         * Create TAP.
         */

        inst.ifp.tap_fd =
            tap_alloc(TAP_B);


        /*
         * Give TAP its explicit MAC address.
         */

        set_interface_mac(TAP_B,
                           MAC_B);


        /*
         * Configure Linux networking.
         */

        configure_instance_B();


        /*
         * Create raw Ethernet transport.
         */

        inst.ifp.raw_fd =
            raw_socket_create(VETH_B);


        run_instance_B(&inst);
    }


    return EXIT_SUCCESS;
}
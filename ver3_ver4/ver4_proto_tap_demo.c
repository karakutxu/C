#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>

#include <linux/if_tun.h>
#include <linux/if_ether.h>


#define TAP_NAME       "tap0"
#define FINAL_IP       "10.0.1.99"

static volatile sig_atomic_t ready;


/*
 * The higher layer/test script has finished configuring Linux.
 */
static void ready_handler(int sig)
{
    (void)sig;
    ready = 1;
}


/*
 * Production-relevant TAP creation.
 *
 * No IP configuration.
 * No routing.
 * No neighbour configuration.
 * No netlink.
 */
static int tap_alloc(const char *name)
{
    struct ifreq ifr;
    int fd;

    fd = open("/dev/net/tun", O_RDWR);

    if (fd < 0) {
        perror("open /dev/net/tun");
        exit(EXIT_FAILURE);
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("TUNSETIFF");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}


static void print_mac(const unsigned char *mac)
{
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}


static void read_tap_frame(int tap_fd)
{
    unsigned char frame[2048];
    int len;

    struct ethhdr *eth;
    struct iphdr *ip;

    char dst_ip[INET_ADDRSTRLEN];


    /*
     * TAP gives us the complete Ethernet frame.
     */
    len = read(tap_fd, frame, sizeof(frame));

    if (len < 0) {
        perror("read TAP");
        exit(EXIT_FAILURE);
    }

    if (len < (int)(sizeof(struct ethhdr) + sizeof(struct iphdr))) {
        printf("Frame too short\n");
        return;
    }


    eth = (struct ethhdr *)frame;
    ip  = (struct iphdr *)(frame + sizeof(struct ethhdr));


    inet_ntop(AF_INET,
              &ip->daddr,
              dst_ip,
              sizeof(dst_ip));


    printf("\n");
    printf("============================================================\n");
    printf("PROTO READ FROM TAP\n");
    printf("============================================================\n");

    printf("Frame length       : %d bytes\n", len);

    printf("\nEthernet header:\n");

    printf("  Destination MAC  : ");
    print_mac(eth->h_dest);
    printf("\n");

    printf("  Source MAC       : ");
    print_mac(eth->h_source);
    printf("\n");

    /*
     * Demo convention:
     *
     * COMSYS L2 address = last octet of destination MAC.
     */
    printf("  COMSYS L2 address : %u\n",
           eth->h_dest[5]);


    printf("\nIP header:\n");

    printf("  Destination IP   : %s\n",
           dst_ip);


    printf("\nExpected result:\n");

    printf("  Final L3 destination : %s\n",
           FINAL_IP);

    printf("  L2 address            : %u\n",
           eth->h_dest[5]);

    printf("\n");
}


int main(void)
{
    int tap_fd;


    /*
     * Create TAP only.
     *
     * Everything else is configured externally.
     */
    tap_fd = tap_alloc(TAP_NAME);

    printf("Created %s\n", TAP_NAME);
    printf("Waiting for Linux network configuration...\n");
    printf("PID = %d\n", getpid());


    /*
     * Wait for setup_tap_demo.sh.
     */
    signal(SIGUSR1, ready_handler);

    while (!ready)
        pause();


    printf("\nLinux configuration complete.\n");
    printf("Waiting for an Ethernet frame on %s...\n",
           TAP_NAME);


    /*
     * This is the production-relevant operation:
     *
     *     read complete Ethernet frame from TAP.
     */
    read_tap_frame(tap_fd);


    close(tap_fd);

    return EXIT_SUCCESS;
}
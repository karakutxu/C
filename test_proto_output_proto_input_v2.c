#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/ip.h>

#include "proto_tun_tap.h"
#include "test_helpers.h"
#include "tun_tap.h"

#include "mbuf_helpers.h"
#include "ifnet_compat.h"
#include "test_helpers.h"
#include "mbuf_compat.h"

/* Kernel-compatible mtod */
#define mtod(m,t) ((t)((m)->m_data))

#define TEST_PAYLOAD_SIZE 32

uint16_t in_cksum(void *data, int len)
{
    uint32_t sum = 0;
    uint16_t *ptr = data;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    if (len > 0) {
        sum += *((uint8_t *)ptr);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0)
    {
        printf("ERROR: opening TUN \n");
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if (*dev)
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	// Check if ioctl operation permitted by the kernel
	if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0)
	{
		printf("Error creating the tun: TUNSETIFF \n");
		close(fd);
		return -1;
	}

	strcpy(dev, ifr.ifr_name);
	return fd;
}

/* Packet generator with configurable destination */
int build_test_packet(uint8_t *buf, uint32_t src, uint32_t dst)
{
    int ip_p = IPPROTO_ICMP;
    //int ip_p = IPPROTO_UDP;
	ip->ip_p = ip_p;

    struct ip *ip = (struct ip *)buf;
	int total_len = 0;

    memset(buf, 0, RX_LAYER_2_CHUNK_SIZE);

    ip->ip_v = 4;
    ip->ip_hl = 5;
    ip->ip_ttl = 64;
    ip->ip_src.s_addr = htonl(src); // 10.0.0.99
    ip->ip_dst.s_addr = htonl(dst); // 10.0.1.99
    ip->ip_tos = 0;
	ip->ip_id = htons(1);
	ip->ip_off = htons(IP_DF);


	switch (ip_p)
	{
		case IPPROTO_UDP:
		{
			printf("IP Protocol: UDP \n");

			ip->ip_p = IPPROTO_UDP;

			struct udphdr *udp = (struct udphdr *)(buf + sizeof(struct ip));

			udp->source = htons(1234);
			udp->dest   = htons(5678);

			uint8_t *payload = (uint8_t *)(udp + 1);

			for (int i = 0; i < TEST_PAYLOAD_SIZE; i++) {
				payload[i] = (uint8_t)(i + 1);
			}

			int udp_len = sizeof(struct udphdr) + TEST_PAYLOAD_SIZE;

			udp->len = htons(udp_len);
			udp->check = 0; // acceptable for IPv4 (can keep zero)

			ip->ip_len = htons(sizeof(struct ip) + udp_len);
			total_len = sizeof(struct ip) + udp_len;

			break;
		}
		case IPPROTO_ICMP:
		{
			printf("IP Protocol: ICMP \n");

			ip->ip_p = IPPROTO_ICMP;

			struct icmphdr *icmp = (struct icmphdr *)(buf + sizeof(struct ip));

			icmp->type = ICMP_ECHO;
			icmp->code = 0;
			icmp->un.echo.id = htons(1234);
			icmp->un.echo.sequence = htons(1);

			// payload comes after ICMP header
			uint8_t *payload = (uint8_t *)(icmp + 1);

			for (int i = 0; i < TEST_PAYLOAD_SIZE; i++) {
				payload[i] = (uint8_t)(i + 1);
			}

			int icmp_len = sizeof(struct icmphdr) + TEST_PAYLOAD_SIZE;

			icmp->checksum = 0;
			icmp->checksum = in_cksum(icmp, icmp_len);

			ip->ip_len = htons(sizeof(struct ip) + icmp_len);
			total_len = sizeof(struct ip) + icmp_len;

			break;
		}
		default:
			break;
	}

	/* IP header checksum */
	ip->ip_sum = 0;
	ip->ip_sum = in_cksum(ip, sizeof(struct ip));
	printf("IP Packet: checksum = 0x%04x \n", ntohs(ip->ip_sum));

	return total_len;
}

int validate_packet(uint8_t *buf, int len)
{
    struct ip *ip = (struct ip *)buf;

    if (len < (int)sizeof(struct ip)) {
        printf("VALIDATION: packet too short \n");
        return -1;
    }

    /* Validate IP header fields */
    if (ip->ip_v != 4) {
        printf("VALIDATION: wrong IP version \n");
        return -1;
    }

    if (ip->ip_dst.s_addr != htonl(0x0A010063)) {
        printf("VALIDATION: wrong destination \n");
        return -1;
    }

    /* Validate payload */
	uint8_t *payload;
	int payload_len;
	if (ip->ip_p == IPPROTO_ICMP) {
		struct icmphdr *icmp = (struct icmphdr *)(buf + (ip->ip_hl * 4));
		payload = (uint8_t *)(icmp + 1);
		payload_len = len - (ip->ip_hl * 4) - sizeof(struct icmphdr);
	}
	else if (ip->ip_p == IPPROTO_UDP) {
		struct udphdr *udp = (struct udphdr *)(buf + (ip->ip_hl * 4));
		payload = (uint8_t *)(udp + 1);
		payload_len = len - (ip->ip_hl * 4) - sizeof(struct udphdr);
	}
	else {
		printf("VALIDATION: unknown protocol\n");
		return -1;
	}

    if (payload_len < TEST_PAYLOAD_SIZE) {
        printf("VALIDATION: payload short \n");
        return -1;
    }

	for(int i = 0; i < TEST_PAYLOAD_SIZE; i++) {
		if (payload[i] != (uint8_t)(i + 1)) {
			printf("VALIDATION: payload mismatch at %d \n", i);
			return -1;
		}
	}
	return 0;
}

/* TX size (Instance A) */
void test_instance_tx(int fd_tx)
{
    /* Test instance TX */
    /* ------------------ */
    uint8_t txbuff[RX_LAYER_2_CHUNK_SIZE];

    printf("TX: building packet \n");
    int txlen = build_test_packet(txbuff,
        0x0A000063,    // 10.0.0.99
        0x0A010063);   // 10.0.1.99

    /* Print raw bytes of packet */
    printf(" Print raw bytes (tx buffer) ");
    for (int i = 0; i < txlen; i++) {
        printf("%02x ", txbuff[i]);
    }
    printf("\n");

	/* Write packet into TUN */
	printf("TX: writing %d bytes \n", txlen);
	int rc = write(fd_tx, txbuff, txlen);
	if (rc < 0)
		printf("TX: write error, rc = %d \n", rc);
	else
		printf("TX write OK, fd_tx = %d \n", fd_tx);
}

/* RX size (Instance B) */
void test_instance_rx(int fd_rx)
{
    /* Test instance RX */
    /* ------------------ */
    struct ifnet ifp = {0};
    ifp.tun_fd = fd_rx;
    ifp.if_flags = IFF_UP;
    uint8_t rxbuff[RX_LAYER_2_CHUNK_SIZE];

    /* Simulate proto_output_ng */
    // fcntl(fd_rx, F_SETFL, O_NONBLOCK); // non-blocking
    printf("RX: Waiting for packet \n");
	while (1) {
		int len = read(fd_rx, rxbuff, sizeof(rxbuff));
		if (len < 0) {
			printf("RX: read error, rc = %d \n", len);
			return;
		}
		else {
			printf("RX: read packet len = %d \n", len);
		}

		/* Print first 8 bytes */
		printf(" Print first 8 bytes ");
		for (int i = 0; i < 8; i++) {
			printf("%02x ", rxbuff[i]);
		}
		printf("\n");

		/* rebuild expected */
		/*
		uint8_t expected[RX_LAYER_2_CHUNK_SIZE];
		int explen = build_test_packet(expected,
									  0x0A000063,  // 10.0.0.99
									  0x0A010063); // 10.0.1.99
		*/

		/* Validate raw packet */
		/*
		if (len != explen || memcmp(expected, rxBuff, explen) != 0) {
			printf(" TEST1 (A->B): FAILED \n");
		} else {
			printf(" TEST1 (A->B): PASSED \n");
		} */

		if ((validate_packet(rxBuff, len) == 0) {
			printf(" TEST1 (A->B): PASSED \n");
		} else {
			printf(" TEST1 (A->B): FAILED \n");
		}
	}
}

int main(void)
{
    /* Declare and initialize ifp, s */
    struct ifnet *ifp = test_init_ifnet();
    struct proto_stats *s = test_init_proto(ifp);
    ifp->p = s;

	/* To eliminate error RTNETLINK: File exists
	which means that we may be re-adding routes or IPs */
	//system("ip addr flush dev tun0");
	//system("ip addr flush dev tun1");
	
    // Clean up old state
    system("ip link delete tun0 2>/dev/null");
    system("ip link delete tun1 2>/dev/null");

	/* Create TUN interfaces: tun0 and tun1*/
	char device0[IFNAMSIZ] = "tun0";
	char device1[IFNAMSIZ] = "tun1";
	int tun_fd_tx = tun_alloc(device0);
	if (tun_fd_tx < 0)
	{
		printf("Failed to create TUN interface \n");
		return -1;
	}
	int tun_fd_rx = tun_alloc(device1);
	if (tun_fd_rx < 0)
	{
		printf("Failed to create TUN interface \n");
		return -1;
	}

	printf("TEST proto TUN/TAP: Using TUN device: %s \n", device0);
	printf("TEST proto TUN/TAP: Using TUN device: %s \n", device1);

	/* Configure TUN0 interface at 10.0.0.1 */
	 /* Configure TUN1 interface in a DIFFERENT SUBNET */
	system("ip addr add 10.0.0.1/24 tun0");
	system("ip addr add 10.0.1.1/24 tun1");
	system("ip link set tun0 up");
	system("ip link set tun1 up");
	
	// Disable IPv6
	system("sysctl -w net.ipv6.conf.all.disable_ipv6=1");
	system("sysctl -w net.ipv6.conf.default.disable_ipv6=1");

	// Disable reverse path filtering
	system("sysctl -w net.ipv4.conf.all.rp_filter=0");

	system("sysctl -w net.ipv4.conf.all.accept_local=1");
	system("sysctl -w net.ipv4.conf.all.route_localnet=1");

	// Enable routing
	system("sysctl -w net.ipv4.ip_forward=1");

	// Add routes
	system("ip route add 10.0.0.0/24 dev tun0");
	system("ip route add 10.0.1.0/24 dev tun1");

	/* fork() is the simplest (no threads), it creates a child process which
	   is an exact copy of the of the parent process */
	pid_t pid_rx = fork();
	if (pid_rx == 0) {
		// Child --> RX (instance B)
		test_instance_rx(tun_fd_rx);
		return 0;
	}

	while(1) {
		sleep(1); // Let RX block first
		// parent --> TX (instance A)
		test_instance_tx(tun_fd_tx);
	}

	return 0;
}
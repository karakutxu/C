#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <linux/if_tun.h>
#include <net/if.h>
#include <linux/ip.h>

#define MAX_INSTANCES 8
#define MAX_ROUTES 16
#define BUF_SIZE 2000

/* ================= ROUTING ================= */

typedef struct {
    uint32_t dst;
    uint32_t mask;
    int out_instance;
} route_entry_t;

/* ================= INSTANCE ================= */

typedef struct {
    char name[16];
    int id;

    route_entry_t routes[MAX_ROUTES];
    int route_count;

} proto_instance_t;

proto_instance_t instances[MAX_INSTANCES];
int instance_count = 0;

/* ================= TUN ================= */

int tun_fd;

int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd = open("/dev/net/tun", O_RDWR);

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        perror("TUNSETIFF");
        exit(1);
    }

    return fd;
}

/* ================= UTILS ================= */

uint32_t ip_parse(const char *ip)
{
    return inet_addr(ip);
}

void ip_print(uint32_t ip)
{
    struct in_addr a = { .s_addr = ip };
    printf("%s", inet_ntoa(a));
}

/* ================= ROUTING ================= */

int lookup_route(proto_instance_t *inst, uint32_t dst)
{
    for (int i = 0; i < inst->route_count; i++) {
        if ((dst & inst->routes[i].mask) ==
            (inst->routes[i].dst & inst->routes[i].mask)) {
            return inst->routes[i].out_instance;
        }
    }
    return -1;
}

/* ================= PROTO INPUT ================= */

void proto_input(proto_instance_t *inst,
                 unsigned char *buf, int len)
{
    struct iphdr *ip = (struct iphdr *)buf;

    printf("[%s] proto_input: ", inst->name);
    ip_print(ip->saddr);
    printf(" → ");
    ip_print(ip->daddr);
    printf(" proto=%d len=%d\n", ip->protocol, len);
}

/* ================= PROTO OUTPUT ================= */

void proto_output(proto_instance_t *inst,
                  unsigned char *buf, int len)
{
    struct iphdr *ip = (struct iphdr *)buf;

    printf("[%s] proto_output: route lookup for ", inst->name);
    ip_print(ip->daddr);
    printf("\n");

    int out = lookup_route(inst, ip->daddr);

    if (out < 0) {
        printf("[%s] DROP (no route)\n", inst->name);
        return;
    }

    printf("[%s] → forwarding to instance %s\n",
           inst->name, instances[out].name);

    /* Deliver to next instance */
    proto_input(&instances[out], buf, len);
}

/* ================= INSTANCE SETUP ================= */

proto_instance_t *create_instance(const char *name)
{
    proto_instance_t *inst = &instances[instance_count];
    memset(inst, 0, sizeof(*inst));

    strncpy(inst->name, name, sizeof(inst->name));
    inst->id = instance_count;

    instance_count++;
    return inst;
}

void add_route(proto_instance_t *inst,
               const char *dst,
               const char *mask,
               int out_instance)
{
    route_entry_t *r = &inst->routes[inst->route_count++];

    r->dst = ip_parse(dst);
    r->mask = ip_parse(mask);
    r->out_instance = out_instance;
}

/* ================= TEST PACKET ================= */

void build_packet(unsigned char *buf,
                  const char *src,
                  const char *dst)
{
    struct iphdr *ip = (struct iphdr *)buf;

    memset(buf, 0, BUF_SIZE);

    ip->version = 4;
    ip->ihl = 5;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->saddr = ip_parse(src);
    ip->daddr = ip_parse(dst);
    ip->tot_len = htons(sizeof(struct iphdr));
}

/* ================= TESTS ================= */

void test_A_to_B()
{
    printf("\n=== TEST proto_output(A) → proto_input(B) ===\n");

    unsigned char buf[BUF_SIZE];
    build_packet(buf, "10.0.0.1", "10.0.1.1");

    proto_output(&instances[0], buf, sizeof(struct iphdr));
}

void test_B_to_A()
{
    printf("\n=== TEST proto_output(B) → proto_input(A) ===\n");

    unsigned char buf[BUF_SIZE];
    build_packet(buf, "10.0.1.1", "10.0.0.1");

    proto_output(&instances[1], buf, sizeof(struct iphdr));
}

/* ================= MAIN ================= */

int main()
{
    /* Create protocol instances */
    proto_instance_t *A = create_instance("A");
    proto_instance_t *B = create_instance("B");

    /* Configure routing tables (like rtentry) */
    add_route(A, "10.0.1.0", "255.255.255.0", B->id);
    add_route(B, "10.0.0.0", "255.255.255.0", A->id);

    /* Run isolated unit tests */
    test_A_to_B();
    test_B_to_A();

    /* Setup TUN */
    tun_fd = tun_alloc("tun0");

    printf("\n=== LIVE MODE ===\n");

    unsigned char buf[BUF_SIZE];

    while (1) {
        int len = read(tun_fd, buf, sizeof(buf));
        if (len < 0) break;

        printf("\n[TUN] packet received\n");

        /* Inject into instance A (ingress) */
        proto_input(A, buf, len);

        /* Route it */
        proto_output(A, buf, len);

        /* Send back out */
        write(tun_fd, buf, len);
    }

    return 0;
}
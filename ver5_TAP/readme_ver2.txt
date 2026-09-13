ARP should not merely be avoided because a permanent neighbour entry happens to exist; the demo should explicitly disable ARP on the TAP interfaces and install the L2 mappings statically. That makes the intended architecture unambiguous.

The resulting demonstration is:

             Linux namespace ns1
             
       application
           |
           | UDP/IP
           v
     Linux IP routing
           |
           | route to 10.0.1.0/24
           | via 192.168.0.2
           v
     static neighbour entry
     192.168.0.2
           |
           | explicitly configured
           v
     02:00:00:00:00:02
           |
           v
         TAP0
           |
           v
    proto_output()
           |
           | eth->h_dest[5]
           v
      L2 address = 2
           |
           v
        veth0
           |
           | veth pair
           |
        veth1
           |
           v
    proto_input()
           |
           v
         TAP1
           |
           v
     Linux IP stack
           |
           v
       UDP receiver

There is no ARP exchange anywhere in the test.

The static neighbour entries are the Linux-side representation of the explicitly configured L2 information.

Below are the two complete files.

1. setup_netns.sh

2. proto_kernel_test.c

This version deliberately keeps the structure close to the sort of code you're porting:

struct mbuf
struct ifnet
struct proto_instance
proto_output()
proto_input()
TAP as the stack interface
raw Ethernet socket as the transport side

The important difference from the previous code is that Linux's neighbour entry is permanent and ARP is disabled.

One small correction to the source above

There is one include that should be present because fd_set, FD_SET, select() etc. are used. Add:

#include <sys/select.h>

near the other system includes:

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>

I would make that part of the actual source rather than relying on an indirect include.

What this demo now proves

The test has two completely separate address mechanisms.

IP routing

Linux knows:

10.0.1.0/24
       ↓
via 192.168.0.2

through:

ip route replace 10.0.1.0/24 via 192.168.0.2 dev tap0
L2 configuration

Linux is explicitly told:

192.168.0.2
      ↓
02:00:00:00:00:02

through:

ip neigh replace 192.168.0.2 \
    lladdr 02:00:00:00:00:02 \
    nud permanent \
    dev tap0

And:

ip link set dev tap0 arp off

means the interface is explicitly configured not to use ARP.

So there is no:

ARP request
    ↓
Who has 192.168.0.2?
    ↓
ARP reply
    ↓
MAC learned dynamically

Instead:

Static configuration
       ↓
192.168.0.2 → 02:00:00:00:00:02
The critical demonstration

When the application executes:

sendto(..., "TEST", ...);

it specifies only:

source IP      10.0.0.99
destination IP 10.0.1.99

It does not specify a MAC address.

Linux performs the route lookup:

10.0.1.99
    ↓
10.0.1.0/24
    ↓
next hop 192.168.0.2

It then uses the static neighbour entry:

192.168.0.2
      ↓
02:00:00:00:00:02

The resulting Ethernet frame delivered through TAP is therefore:

+--------------------------------------------------+
| Ethernet header                                  |
|                                                  |
| DST = 02:00:00:00:00:02                         |
| SRC = 02:00:00:00:00:01                         |
+--------------------------------------------------+
| IP header                                        |
|                                                  |
| SRC = 10.0.0.99                                  |
| DST = 10.0.1.99                                  |
+--------------------------------------------------+
| UDP                                               |
+--------------------------------------------------+
| TEST                                              |
+--------------------------------------------------+

Then your protocol does:

struct ethhdr *eth =
    (struct ethhdr *)m->data;

uint8_t l2 =
    mac_to_l2(eth->h_dest);

giving:

eth->h_dest
     ↓
02:00:00:00:00:02
     ↓
h_dest[5]
     ↓
2
     ↓
COMSYS L2 address

That is the important correspondence with the legacy code:

LYNXOS                         LINUX DEMO
------                         ----------

IP routing                     IP routing
     |                               |
     v                               v
dst->sa_data[5]                static neighbour
     |                               |
     |                               v
     |                         destination MAC
     |                               |
     v                               v
L2 address                    TAP Ethernet frame
                                     |
                                     v
                              eth->h_dest[5]
                                     |
                                     v
                                L2 address

So ARP is completely outside the model. The only dynamic operation being demonstrated is Linux's IP route selection; the L2 destination is explicitly configured, which is much closer to the networking architecture you're trying to reproduce from the LynxOS code.
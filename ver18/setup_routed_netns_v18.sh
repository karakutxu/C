#!/bin/bash
set -e

#
# cleanup
#

ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true
ip netns del nsr 2>/dev/null || true

#
# namespaces
#

ip netns add ns1
ip netns add ns2
ip netns add nsr

#
# veth pair ns1 <-> router
#

ip link add veth1 type veth peer name vethr1

ip link set veth1 netns ns1
ip link set vethr1 netns nsr

#
# veth pair router <-> ns2
#

ip link add veth2 type veth peer name vethr2

ip link set veth2 netns ns2
ip link set vethr2 netns nsr

#
# addresses
#

ip netns exec ns1 \
    ip addr add 10.0.1.1/24 dev veth1

ip netns exec nsr \
    ip addr add 10.0.1.254/24 dev vethr1

ip netns exec ns2 \
    ip addr add 10.0.2.1/24 dev veth2

ip netns exec nsr \
    ip addr add 10.0.2.254/24 dev vethr2

#
# links up
#

ip netns exec ns1 ip link set lo up
ip netns exec ns2 ip link set lo up
ip netns exec nsr ip link set lo up

ip netns exec ns1 ip link set veth1 up
ip netns exec ns2 ip link set veth2 up

ip netns exec nsr ip link set vethr1 up
ip netns exec nsr ip link set vethr2 up

#
# routes
#

ip netns exec ns1 \
    ip route add default via 10.0.1.254

ip netns exec ns2 \
    ip route add default via 10.0.2.254

#
# enable routing
#

ip netns exec nsr \
    sysctl -w net.ipv4.ip_forward=1 >/dev/null

echo
echo "===================================="
echo " Routed namespaces ready"
echo "===================================="
echo
echo "ns1     : 10.0.1.1"
echo "router  : 10.0.1.254 / 10.0.2.254"
echo "ns2     : 10.0.2.1"
echo
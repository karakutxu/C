#!/bin/bash
set -e

#
# proto TAP/L2 routing demonstration
#
# Topology:
#
#       ns1                              ns2
#
#    tap0 192.168.0.1/24             tap1 192.168.0.2/24
#    10.0.0.99/24                    10.0.1.99/24
#       |                                  |
#     proto A                            proto B
#       |                                  |
#     veth0 -------------------------- veth1
#
# Linux performs:
#
#   IP routing
#   ARP/neighbour resolution
#
# proto sees the resulting Ethernet frame through TAP.
#

echo "Cleaning up old namespaces..."

ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true

echo "Creating namespaces..."

ip netns add ns1
ip netns add ns2

echo "Creating veth pair..."

ip link add veth0 type veth peer name veth1

echo "Moving veth interfaces into namespaces..."

ip link set veth0 netns ns1
ip link set veth1 netns ns2

#
# veth interfaces deliberately have NO IP addresses.
#
# They are simply the L2 transport between proto A and proto B.
#

ip netns exec ns1 ip link set lo up
ip netns exec ns2 ip link set lo up

ip netns exec ns1 ip link set veth0 up
ip netns exec ns2 ip link set veth1 up

#
# Enable promiscuous mode so the raw packet socket used by proto
# can see frames whose destination MAC belongs to the TAP interface
# rather than to the veth interface itself.
#

ip netns exec ns1 ip link set veth0 promisc on
ip netns exec ns2 ip link set veth1 promisc on

echo
echo "Namespaces ready."
echo
echo "Start receiver first:"
echo
echo "  sudo ip netns exec ns2 ./proto_kernel_test B"
echo
echo "Then sender:"
echo
echo "  sudo ip netns exec ns1 ./proto_kernel_test A"
echo
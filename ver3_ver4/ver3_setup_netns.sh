#!/bin/bash

set -e

# ------------------------------------------------------------
# Clean up previous test
# ------------------------------------------------------------

ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true


# ------------------------------------------------------------
# Create namespaces and protocol transport
# ------------------------------------------------------------

ip netns add ns1
ip netns add ns2

ip link add veth0 type veth peer name veth1

ip link set veth0 netns ns1
ip link set veth1 netns ns2

ip -n ns1 link set lo up
ip -n ns2 link set lo up

ip -n ns1 link set veth0 up
ip -n ns2 link set veth1 up

# The proto raw socket receives frames from the veth.
# Promiscuous mode lets it receive frames whose destination
# MAC is the TAP MAC rather than the veth MAC.

ip -n ns1 link set veth0 promisc on
ip -n ns2 link set veth1 promisc on


# ------------------------------------------------------------
# TAP interfaces
#
# These are created by the C program with TUNSETIFF.
#
# After the C program has created them, this script continues
# below and configures them.
# ------------------------------------------------------------

echo
echo "Compile first:"
echo
echo "  gcc -Wall -Wextra -O0 -g proto_kernel_test.c -o proto_kernel_test"
echo
echo "Now run:"
echo
echo "  sudo ip netns exec ns1 ./proto_kernel_test A &"
echo "  sudo ip netns exec ns2 ./proto_kernel_test B"
echo
echo "The C program will create tap0/tap1."
echo "Then run the following configuration commands manually"
echo "inside the namespaces:"
echo
echo "  ip -n ns1 link set tap0 address 02:00:00:00:00:01"
echo "  ip -n ns1 link set tap0 up"
echo "  ip -n ns1 addr add 10.0.0.1/24 dev tap0"
echo "  ip -n ns1 addr add 192.168.0.1/24 dev tap0"
echo "  ip -n ns1 route add 10.0.1.0/24 via 192.168.0.2 dev tap0"
echo "  ip -n ns1 neigh replace 192.168.0.2 lladdr 02:00:00:00:00:02 nud permanent dev tap0"
echo
echo "  ip -n ns2 link set tap1 address 02:00:00:00:00:02"
echo "  ip -n ns2 link set tap1 up"
echo "  ip -n ns2 addr add 10.0.1.99/24 dev tap1"
echo "  ip -n ns2 addr add 192.168.0.2/24 dev tap1"
echo
#!/bin/bash
set -e

#
# ============================================================
# PROTO Linux/TAP Layer-2 demonstration
# ============================================================
#
# This demo models a networking system where:
#
#   - there is no ARP
#   - Layer-2 addresses are configured explicitly
#   - there is a fixed MAC <-> L2 mapping
#   - the last MAC octet is the L2 address
#
# The Linux IP stack performs normal IP routing.
#
# Linux does NOT perform ARP.
#
# Instead, static neighbour entries are installed explicitly:
#
#     192.168.0.2 -> 02:00:00:00:00:02
#     192.168.0.1 -> 02:00:00:00:00:01
#
# The complete Ethernet frame is then delivered to the
# userspace protocol through TAP.
#
# Topology:
#
#
#       ns1                                      ns2
#
#   10.0.0.99/24                            10.0.1.99/24
#   192.168.0.1/24                          192.168.0.2/24
#         |                                      |
#        tap0                                  tap1
#         |                                      |
#      proto A                                proto B
#         |                                      |
#       veth0 -------------------------------- veth1
#
#
# Linux routing:
#
# ns1:
#
#   10.0.1.0/24 via 192.168.0.2 dev tap0
#
# ns2:
#
#   10.0.0.0/24 via 192.168.0.1 dev tap1
#
#
# Explicit L2 configuration:
#
# ns1:
#
#   192.168.0.2 -> 02:00:00:00:00:02
#
# ns2:
#
#   192.168.0.1 -> 02:00:00:00:00:01
#
#
# Fixed demonstration mapping:
#
#   MAC 02:00:00:00:00:N -> L2 address N
#
# ============================================================


echo
echo "============================================"
echo " PROTO TAP/L2 network setup"
echo "============================================"
echo


# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------

echo "Removing old namespaces..."

ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true


# ------------------------------------------------------------
# Create namespaces
# ------------------------------------------------------------

echo "Creating namespaces..."

ip netns add ns1
ip netns add ns2


# ------------------------------------------------------------
# Create veth pair
# ------------------------------------------------------------

echo "Creating veth pair..."

ip link add veth0 type veth peer name veth1


# ------------------------------------------------------------
# Move each veth into its namespace
# ------------------------------------------------------------

echo "Moving veth interfaces into namespaces..."

ip link set veth0 netns ns1
ip link set veth1 netns ns2


# ------------------------------------------------------------
# Bring up loopback
# ------------------------------------------------------------

ip netns exec ns1 ip link set lo up
ip netns exec ns2 ip link set lo up


# ------------------------------------------------------------
# Bring up veth interfaces
#
# There are deliberately NO IP addresses on these
# interfaces.
#
# They provide only the Ethernet transport between the two
# protocol instances.
# ------------------------------------------------------------

ip netns exec ns1 ip link set veth0 up
ip netns exec ns2 ip link set veth1 up


# ------------------------------------------------------------
# Put veth interfaces into promiscuous mode.
#
# The Ethernet destination MAC belongs to tap0/tap1, not
# to veth0/veth1.
#
# proto uses an AF_PACKET socket on the veth to transport
# the complete Ethernet frame.
# ------------------------------------------------------------

ip netns exec ns1 ip link set veth0 promisc on
ip netns exec ns2 ip link set veth1 promisc on


echo
echo "============================================"
echo " Network ready"
echo "============================================"
echo
echo "Compile:"
echo
echo "    gcc -Wall -Wextra -O0 -g \\"
echo "        proto_kernel_test.c \\"
echo "        -o proto_kernel_test"
echo
echo
echo "Terminal 1:"
echo
echo "    sudo ip netns exec ns2 ./proto_kernel_test B"
echo
echo "Terminal 2:"
echo
echo "    sudo ip netns exec ns1 ./proto_kernel_test A"
echo
echo
echo "The receiver should be started first."
echo
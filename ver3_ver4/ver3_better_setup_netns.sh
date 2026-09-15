#!/bin/bash

set -e

# ------------------------------------------------------------
# Clean up
# ------------------------------------------------------------

ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true

# ------------------------------------------------------------
# Create namespaces
# ------------------------------------------------------------

ip netns add ns1
ip netns add ns2

# ------------------------------------------------------------
# Create protocol transport
# ------------------------------------------------------------

ip link add veth0 type veth peer name veth1

ip link set veth0 netns ns1
ip link set veth1 netns ns2

ip -n ns1 link set lo up
ip -n ns2 link set lo up

ip -n ns1 link set veth0 up
ip -n ns2 link set veth1 up

ip -n ns1 link set veth0 promisc on
ip -n ns2 link set veth1 promisc on


# ------------------------------------------------------------
# Start the two proto instances.
#
# They ONLY create TAP and then wait.
# ------------------------------------------------------------

ip netns exec ns1 ./proto_kernel_test A &
PID_A=$!

ip netns exec ns2 ./proto_kernel_test B &
PID_B=$!


# ------------------------------------------------------------
# Wait for TAP interfaces to appear
# ------------------------------------------------------------

echo "Waiting for TAP interfaces..."

for i in {1..50}; do

    if ip -n ns1 link show tap0 >/dev/null 2>&1 &&
       ip -n ns2 link show tap1 >/dev/null 2>&1
    then
        break
    fi

    sleep 0.1
done


# ------------------------------------------------------------
# TAP A
#
# MAC last octet = COMSYS L2 address 1
# ------------------------------------------------------------

ip -n ns1 link set tap0 address 02:00:00:00:00:01
ip -n ns1 link set tap0 arp off
ip -n ns1 link set tap0 up

ip -n ns1 addr add 10.0.0.1/24 dev tap0
ip -n ns1 addr add 192.168.0.1/24 dev tap0


# ------------------------------------------------------------
# TAP B
#
# MAC last octet = COMSYS L2 address 2
# ------------------------------------------------------------

ip -n ns2 link set tap1 address 02:00:00:00:00:02
ip -n ns2 link set tap1 arp off
ip -n ns2 link set tap1 up

ip -n ns2 addr add 10.0.1.99/24 dev tap1
ip -n ns2 addr add 192.168.0.2/24 dev tap1


# ------------------------------------------------------------
# ROUTING
#
# Final destination:
#
#     10.0.1.99
#
# Next hop:
#
#     192.168.0.2
# ------------------------------------------------------------

ip -n ns1 route add 10.0.1.0/24 \
    via 192.168.0.2 dev tap0


# ------------------------------------------------------------
# STATIC L2 MAPPING
#
# No ARP.
#
# Linux is explicitly told:
#
#     next-hop 192.168.0.2
#         ->
#     MAC 02:00:00:00:00:02
# ------------------------------------------------------------

ip -n ns1 neigh replace \
    192.168.0.2 \
    lladdr 02:00:00:00:00:02 \
    nud permanent \
    dev tap0


# ------------------------------------------------------------
# Show exactly what Linux has configured
# ------------------------------------------------------------

echo
echo "========== ns1 =========="

ip -n ns1 addr show dev tap0
ip -n ns1 route show
ip -n ns1 neigh show dev tap0

echo
echo "========== ns2 =========="

ip -n ns2 addr show dev tap1
ip -n ns2 route show


# ------------------------------------------------------------
# Tell proto instances that configuration is complete.
#
# For this simple demo, create a marker file in each namespace.
# ------------------------------------------------------------

ip netns exec ns1 sh -c 'touch /tmp/proto_ready'
ip netns exec ns2 sh -c 'touch /tmp/proto_ready'


# ------------------------------------------------------------
# Wait for proto A to finish
# ------------------------------------------------------------

wait $PID_A
wait $PID_B
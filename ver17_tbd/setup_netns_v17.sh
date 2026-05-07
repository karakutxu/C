#!/bin/bash
set -e

ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true

ip netns add ns1
ip netns add ns2

ip link add veth0 type veth peer name veth1

ip link set veth0 netns ns1
ip link set veth1 netns ns2

ip netns exec ns1 ip addr add 10.0.0.1/24 dev veth0
ip netns exec ns2 ip addr add 10.0.0.2/24 dev veth1

ip netns exec ns1 ip link set lo up
ip netns exec ns2 ip link set lo up

ip netns exec ns1 ip link set veth0 up
ip netns exec ns2 ip link set veth1 up

ip netns exec ns1 sysctl -w net.ipv4.ip_forward=1 >/dev/null
ip netns exec ns2 sysctl -w net.ipv4.ip_forward=1 >/dev/null

# static routes

ip netns exec ns1 ip route add 10.0.0.2 dev veth0
ip netns exec ns2 ip route add 10.0.0.1 dev veth1

echo "Namespaces ready"
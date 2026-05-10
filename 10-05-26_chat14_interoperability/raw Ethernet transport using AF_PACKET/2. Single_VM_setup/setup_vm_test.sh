#!/bin/bash
set -e

ip netns del nsA 2>/dev/null || true
ip netns del nsB 2>/dev/null || true

ip netns add nsA
ip netns add nsB

ip link add vethA type veth peer name vethB

ip link set vethA netns nsA
ip link set vethB netns nsB

ip netns exec nsA ip addr add 192.168.100.1/24 dev vethA
ip netns exec nsB ip addr add 192.168.100.2/24 dev vethB

ip netns exec nsA ip link set lo up
ip netns exec nsB ip link set lo up

ip netns exec nsA ip link set vethA up
ip netns exec nsB ip link set vethB up

echo DONE
#!/bin/bash
set -e

ip netns del nsA 2>/dev/null || true
ip netns del nsB 2>/dev/null || true

ip netns add nsA
ip netns add nsB

ip link add vethA type veth peer name vethB

ip link set vethA netns nsA
ip link set vethB netns nsB

ip netns exec nsA ip link set lo up
ip netns exec nsB ip link set lo up

ip netns exec nsA ip addr add 192.168.100.1/24 dev vethA
ip netns exec nsB ip addr add 192.168.100.2/24 dev vethB

ip netns exec nsA ip link set vethA up
ip netns exec nsB ip link set vethB up

ip netns exec nsA ip tuntap add dev tun0 mode tun
ip netns exec nsB ip tuntap add dev tun0 mode tun

# With TUN devices Linux expects a virtual point-to-point link !
# But for TUN devices, Linux behaves much more reliably with
# point-to-point addressing (2nd )
# instead of subnet addressing (1st)
ip netns exec nsA ip addr add 10.10.0.1/24 dev tun0
#ip netns exec nsA ip addr add 10.10.0.1 peer 10.20.0.1 dev tun0
ip netns exec nsB ip addr add 10.20.0.1/24 dev tun0
#ip netns exec nsB ip addr add 10.20.0.1 peer 10.10.0.1 dev tun0

ip netns exec nsA ip link set tun0 up
ip netns exec nsB ip link set tun0 up

# see changes above: removed these lines below if 2nd cmd applied
# because point-to-point TUN automatically installs host routes.
ip netns exec nsA ip route add 10.20.0.0/24 dev tun0
ip netns exec nsB ip route add 10.10.0.0/24 dev tun0

ip netns exec nsA sysctl -w net.ipv4.conf.all.rp_filter=0
ip netns exec nsB sysctl -w net.ipv4.conf.all.rp_filter=0

echo "READY"

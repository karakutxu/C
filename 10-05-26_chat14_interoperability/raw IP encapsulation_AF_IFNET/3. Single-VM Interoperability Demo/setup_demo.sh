#!/bin/bash
set -e

#
# cleanup
#

ip netns del nsA 2>/dev/null || true
ip netns del nsB 2>/dev/null || true

#
# namespaces
#

ip netns add nsA
ip netns add nsB

#
# UNDERLAY transport network
# (represents physical Ethernet/IP network)
#

ip link add vethA type veth peer name vethB

ip link set vethA netns nsA
ip link set vethB netns nsB

ip netns exec nsA ip link set lo up
ip netns exec nsB ip link set lo up

#
# underlay IP addresses
#

ip netns exec nsA ip addr add 192.168.100.1/24 dev vethA
ip netns exec nsB ip addr add 192.168.100.2/24 dev vethB

ip netns exec nsA ip link set vethA up
ip netns exec nsB ip link set vethB up

#
# overlay TUN interfaces
#

ip netns exec nsA ip tuntap add dev tun0 mode tun
ip netns exec nsB ip tuntap add dev tun0 mode tun

#
# overlay addresses
#

ip netns exec nsA ip addr add 10.10.0.1/24 dev tun0
ip netns exec nsB ip addr add 10.20.0.1/24 dev tun0

ip netns exec nsA ip link set tun0 up
ip netns exec nsB ip link set tun0 up

#
# overlay routing
#

ip netns exec nsA ip route add 10.20.0.0/24 dev tun0
ip netns exec nsB ip route add 10.10.0.0/24 dev tun0

#
# disable rp_filter
#

ip netns exec nsA sysctl -w net.ipv4.conf.all.rp_filter=0
ip netns exec nsB sysctl -w net.ipv4.conf.all.rp_filter=0

ip netns exec nsA sysctl -w net.ipv4.conf.vethA.rp_filter=0
ip netns exec nsB sysctl -w net.ipv4.conf.vethB.rp_filter=0

ip netns exec nsA sysctl -w net.ipv4.conf.tun0.rp_filter=0
ip netns exec nsB sysctl -w net.ipv4.conf.tun0.rp_filter=0

echo "READY"

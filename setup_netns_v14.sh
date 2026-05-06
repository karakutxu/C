#!/bin/bash
set -e

ip netns exec ns1 ip tuntap add dev tun0 mode tun
ip netns exec ns1 ip addr add 172.16.0.1/24 dev tun0
ip netns exec ns1 ip link set tun0 up

ip netns exec ns2 ip tuntap add dev tun1 mode tun
ip netns exec ns2 ip addr add 172.16.0.2/24 dev tun1
ip netns exec ns2 ip link set tun1 up

ip netns exec ns1 ip route add 172.16.0.0/24 via 10.0.0.2
ip netns exec ns2 ip route add 172.16.0.0/24 via 10.0.0.1

echo "TUN interfaces configured"
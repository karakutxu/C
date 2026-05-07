##!/bin/bash
set -e

#
# ns1
#

ip netns exec ns1 ip tuntap add dev tun0 mode tun

ip netns exec ns1 \
    ip addr add 172.16.1.1 peer 172.16.2.1 dev tun0

ip netns exec ns1 ip link set tun0 up

#
# ns2
#

ip netns exec ns2 ip tuntap add dev tun1 mode tun

ip netns exec ns2 \
    ip addr add 172.16.2.1 peer 172.16.1.1 dev tun1

ip netns exec ns2 ip link set tun1 up

#
# forwarding
#

ip netns exec ns1 sysctl -w net.ipv4.ip_forward=1
ip netns exec ns2 sysctl -w net.ipv4.ip_forward=1

echo "TUN interfaces configured"

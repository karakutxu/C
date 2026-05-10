#!/bin/bash
set -e

ip tuntap add dev tun0 mode tun
ip addr add 10.20.0.1/24 dev tun0
ip link set tun0 up

ip route add 10.10.0.0/24 dev tun0

sysctl -w net.ipv4.ip_forward=1
sysctl -w net.ipv4.conf.all.rp_filter=0
sysctl -w net.ipv4.conf.tun0.rp_filter=0
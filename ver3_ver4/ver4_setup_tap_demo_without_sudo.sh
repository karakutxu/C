#!/bin/bash

set -e

TAP=tap0
TAP_IP=192.168.0.1
TAP_MAC=02:00:00:00:00:01

NEXT_HOP_IP=192.168.0.2
NEXT_HOP_MAC=02:00:00:00:00:02

FINAL_IP=10.0.1.99

./proto_tap_demo &
PID=$!

sleep 1

ip link set dev $TAP address $TAP_MAC
ip link set dev $TAP arp off
ip addr add $TAP_IP/24 dev $TAP
ip link set dev $TAP up

ip route add $FINAL_IP/32 \
    via $NEXT_HOP_IP \
    dev $TAP

ip neigh replace \
    $NEXT_HOP_IP \
    lladdr $NEXT_HOP_MAC \
    nud permanent \
    dev $TAP

echo
echo "========== TAP =========="
ip addr show dev $TAP

echo
echo "========== ROUTE =========="
ip route get $FINAL_IP

echo
echo "========== NEIGHBOUR =========="
ip neigh show dev $TAP

echo
echo "========== START TEST =========="

kill -USR1 $PID

wait $PID
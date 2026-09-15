#!/bin/bash

set -e

TAP=tap0
TAP_IP=192.168.0.1
TAP_MAC=02:00:00:00:00:01

NEXT_HOP_IP=192.168.0.2
NEXT_HOP_MAC=02:00:00:00:00:02

FINAL_IP=10.0.1.99

# ------------------------------------------------------------
# Create TAP
# ------------------------------------------------------------

sudo ./proto_tap_demo &
PID=$!

# Give the C program time to create TAP.
sleep 1


# ------------------------------------------------------------
# Configure TAP
#
# All Linux networking configuration is deliberately here.
# In production this would be done by the higher layer using
# netlink.
# ------------------------------------------------------------

sudo ip link set dev $TAP address $TAP_MAC

sudo ip link set dev $TAP arp off

sudo ip addr add $TAP_IP/24 dev $TAP

sudo ip link set dev $TAP up


# ------------------------------------------------------------
# Route final destination via next-hop
# ------------------------------------------------------------

sudo ip route add $FINAL_IP/32 \
    via $NEXT_HOP_IP \
    dev $TAP


# ------------------------------------------------------------
# Explicit next-hop -> L2 mapping
#
# No ARP is used.
#
# 192.168.0.2 -> 02:00:00:00:00:02
#                                  ^
#                                  |
#                            COMSYS L2 = 2
# ------------------------------------------------------------

sudo ip neigh replace \
    $NEXT_HOP_IP \
    lladdr $NEXT_HOP_MAC \
    nud permanent \
    dev $TAP


# ------------------------------------------------------------
# Display configuration
# ------------------------------------------------------------

echo
echo "============================================================"
echo "TAP configuration"
echo "============================================================"

ip addr show dev $TAP

echo
echo "Routing:"
ip route get $FINAL_IP

echo
echo "Neighbour:"
ip neigh show dev $TAP

echo
echo "============================================================"
echo "Starting test traffic"
echo "============================================================"


# ------------------------------------------------------------
# Tell C program that Linux configuration is complete.
# ------------------------------------------------------------

kill -USR1 $PID

wait $PID
TUN tests

We do not fake the TUN interface.

Linux already gives us everything we need.

Create

tun_test0

Inject packets with

write(tun_fd,...)

Verify

proto_output_ng()

called correctly.

Conversely

inject mbufs

verify

write()

back into the TUN.
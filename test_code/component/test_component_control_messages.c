Control message tests

One nice thing about your dispatcher is that all IOCTLs share the same code path.

Rather than writing 25 identical tests I'd write one parameterised Unity test.

ENABLE_LINK

DISABLE_LINK

DELETE_LINK

FLUSH

SET_LOCAL_ADDRESS

SET_MTU

SET_PRIORITY

...


Each test checks

correct dispatcher selection
correct message ID
correct call to protoman_socket()

That covers every control command with very little duplicated code.
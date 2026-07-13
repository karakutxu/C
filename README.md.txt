# PRODRV Test Suite

This repository contains the complete Unity/Ceedling test suite for the
socket-based PRODRV architecture.

The philosophy is:

- use REAL AF_UNIX sockets
- avoid mocking send()/recv()
- test complete message paths
- keep handlers unit testable

Repository structure

```
test/

    unit/

    integration/

    mocks/

    support/
```

## Running

```
ceedling test:all
```

Coverage

```
ceedling gcov:all
```

The integration tests create temporary UNIX sockets under

```
/tmp/prodrv_test/
```

which are automatically cleaned up.

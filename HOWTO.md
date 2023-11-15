Writing a simple test
=====================

The following steps show how to use tela to implement a simple test that
performs a network ping to a remote system.

Note: A networked system accessible via SSH is required to actually run
the test.

### 1. Create test directory

```
$ mkdir test
$ cd test
```

### 2. Obtain tela framework

```
$ git clone https://github.com/ibm-s390-linux/tela.git
```

### 3. Create Makefile

Create a Makefile with the following contents:

```Makefile
include tela/tela.mak

TESTS := ping.sh
```

This Makefile tells tela that there will be a single test program named
`ping.sh`.

### 4. Define testcase requirements

Place the following YAML text into a file named `ping.sh.yaml`:

```YAML
system localhost:
  tools:
    ping:
system target:
```

This YAML file tells tela that our test needs two test systems:

  1. The local system (localhost) on which the test program runs. The
     `ping` tool must be available on this system.
  2. A remote system to which we can send a ping. We name the remote system
     "target".

### 5. Create test program

Create a Bash test program named `ping.sh`:

```Bash
#!/bin/bash

REMOTE_IP="$TELA_SYSTEM_target_IP"
echo "Pinging remote IP $REMOTE_IP"
ping -W 1 -c 1 "$REMOTE_IP" || exit 1
exit 0
```

Make sure to set the executable file permission:

```
$ chmod u+x ping.sh
```

When run, this test program performs a single, one-second ping to the IP
address of the target system. The IP address of the target system is provided
by tela via the environment variable `TELA_SYSTEM_target_IP`.

The program reports its result via the program exit code: 0 for success, 1 for
failure.

### 6. Create resource file

Place the following YAML text into a file named `resources.yaml` while
replacing `<hostname>` and `<username>` with the corresponding SSH access
data for the networked system to use:

```YAML
system remote_host:
  ssh:
    host: <hostname>
    user: <username>
```

### 7. Run the test

Use the following command line to run the test:

```
$ make check TELA_RC=resources.yaml
Running 1 tests
(1/1) ping.sh ....................... [pass]
1 tests executed, 1 passed, 0 failed, 0 skipped
Result log stored in /home/user/test/test.log
```

The output above indicates that the test ran successfully. More detailed
information including the output of the test program can be found in the
referenced test.log file:

```
ok     1 - ping.sh
  ---
  testresult: "pass"
  testexec: "/path/to/test/ping.sh"
  exitcode: 0
  ...
  output: |
    [   0.002529] stdout: Pinging remote IP 10.0.0.2
    [   0.006926] stdout: PING 10.0.0.2 (10.0.0.2) 56(84) bytes of data.
    [   0.006926] stdout: 64 bytes from 10.0.0.2: icmp_seq=1 ttl=59 time=1.82 ms
    [   0.006953] stdout:
    [   0.006990] stdout: --- 10.0.0.2 ping statistics ---
    [   0.006990] stdout: 1 packets transmitted, 1 received, 0% packet loss, time 0ms
    [   0.006990] stdout: rtt min/avg/max/mdev = 1.816/1.816/1.816/0.000 ms
  ...

```

Additional information
----------------------

 * [Reference information for writing tests](doc/writing_tests.md)
 * [Test resources explained](doc/resources.md)
 * [The system resource](doc/resources/system.md)

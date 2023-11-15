Test resources
==============

This document contains overview information about test resource requirements.
See also the files in the 'doc/resources/' subdirectory for documentation on
specific resource types.


# About test resource requirements

Some tests may have specific requirements on the test environment, for example:

  - The test must be run in a specific hypervisor
  - The test program must run as root user
  - An I/O device of specific type must be available

tela provides a mechanism for test authors to specify such requirements for
a test program. During a test run, the requirements of a test program are
matched against the list of resources available on the test system.

If there is a match, information about the identity of each requested resource
is made available to the test program via environment variables and a YAML file.
If there is no match, tela will skip the test program with an indication of the
missing resource.

Example:

Test YAML file:

```YAML
system:
  hypervisor:
    type: zvm
  user: root
  dasd mydasd:
    size: >=1GiB
```

This test YAML file specifies that the test must be run on a z/VM guest
as root user, and that a DASD storage device with at least 1GiB size is
required.

Test environment file (contains the list of available resources):

```YAML
system:
  dasd 0.0.1000:
```

This test environment file specifies that there is a DASD storage device with
the specified ID on the local system that may be used for testing.

When run, tela will automatically determine the characteristics of the
resources specified in the test environment file (e.g. the hypervisor type of
the local system, or the size of the DASD device). Assuming that the local
system is a z/VM guest and the test is run as root user, tela will then run the
test program and set the following environment variables:

```
TELA_SYSTEM_DASD_mydasd=0.0.ca00
TELA_SYSTEM_HYPERVISOR_TYPE=zvm
TELA_SYSTEM_HYPERVISOR_VERSION=6.4.0
TELA_SYSTEM_USER=root
```

In addition, the same data will be made available in YAML format in a file
that can be accessed via the TELA\_RESOURCE\_FILE environment variable:

```YAML
system:
  _id: localhost
  hypervisor:
    type: zvm
    version: 6.4.0
  user: root
  dasd mydasd:
    _id: 0.0.ca00
```


If any of the requirements could not be met, the test program is not run and
tela automatically generates a skip result:

```
$ make check
Running 1 tests
(1/1) test ........................ [skip] Missing user: root
1 tests executed, 0 passed, 0 failed, 1 skipped
```

# Working with test requirements

Test authors can specify test requirements by listing required resource objects
and attribute conditions in a test YAML file. The top-level resource object
that can be specified is a system (see [system description](resources/system.md)).

### Attributes

Each resource object may provide a number of attributes (such as the size of
a storage device). Test authors can define conditions for each attribute to
further refine a test requirement.

Example:
```YAML
system:
  dasd mydasd:
    size: >=1GiB
```

This example specifies that a DASD with a value for attribute 'size' of at
least 1GiB is required.

### Attribute conditions

Conditions are specified as

```
  <attribute>: [<operator>] <value>
```

where

  - **`<attribute>`** specifies the attribute name
  - **`<operator>`** is an optional comparison operator that determines how
    the attribute value should be compared to the specified value. The list of
    supported operators is determined by the attribute type (see below).
    The default operator is 'equal to' if none is specified.
  - **`<value>`** is the value to compare the attribute value against.

### Attribute types

A type is associated with each attribute of a resource object. This type
determines the type of conditions that can be specified in a test YAML file
for that attribute.

Supported types are:
  - **scalar:** This attribute contains a textual value. The only supported
    comparison operator is != (not equal).
  - **number:** The attribute contains an integer number. If the number starts
    with '0x', it is interpreted as a hexadecimal number. If it starts with a
    leading '0', it is interpreted as octal number. Otherwise it is interpreted
    as a decimal number.

    The number may be followed by a unit prefix. Supported unit prefixes are
    decimal prefixes:

      - K: 1,000
      - M: 1,000,000 (1000^2)
      - G: 1,000,000,000 (1000^3)
      - T: 1,000,000,000,000 (1000^4)

    and binary prefixes:

      - Ki: 1,024
      - Mi: 1,048,576 (1024^2)
      - Gi: 1,073,741,824 (1024^3)
      - Ti: 1,099,511,627,776 (1024^4)

    Any unit following a number or prefix (such as B for byte) is ignored in
    comparisons.

    Supported comparison  operators are <, <=, >, >=, and !=, corresponding to
    'lower than', 'lower or equal', 'greater than', 'greater or equal', and
    'not equal'.
  - **version:** The attribute contains a version number. Version numbers
    consist of a list of numbers separated by ., -, or _. Supported comparison
    operators are <, <=, >, >=, and !=.

### Attribute variables

Test authors can specify relations between attributes of different objects
using so-called 'attribute variables'. An attribute variable is a symbolic
name that is specified in a test YAML file in place of an actual attribute
value. Example:

```
  <attribute>: %{<name>}
```

where `<attribute>` is the name of a resource attribute and `<name>` is the
variable name.

On its first occurrence in the test YAML file, the variable is initialized with
the attribute value of a matching object. In later occurrences, the variable
is expanded with its assigned value before evaluating attribute conditions.

Example:

```YAML
system:
  dasd a:
    size: %{size_a}
  dasd b:
    size: > %{size_a}
```

This example specifies that two DASD resources 'dasd a' and 'dasd b' are
required. The size of 'dasd b' must be greater than the size of 'dasd a'.

Note: It is not possible to specify a condition for the first occurrence of an
attribute variable in a test YAML file.

### Resource object names

Test authors can choose arbitrary, alpha-numerical names for each resource
they specify in a test YAML file. Test programs can then determine the identity
of a matching resource object using this name.

Example:
```YAML:
system:
  dasd a:
  dasd b:
```

The test program can determine the ID of each DASD during runtime using the
following environment variables:

```
TELA_SYSTEM_DASD_a
TELA_SYSTEM_DASD_b
```

Note:
  - All non-alpha-numerical characters in a resource object name are
    replaced by the underscore character ('\_') in the corresponding environment
    variable name.
  - Names should be chosen to be unique within the same object type, but the
    same name can be re-used for different object types (e.g. "dasd a" and
    "zfcp-host a").
  - The object name is only used for generating the environment variable name.
  - There is no dependency between names of child and parent objects.

### Wildcard resource requirements

Test authors can use the shell wildcard character '\*' in place of a resource
name in a test YAML file. As a result, all matching resource objects will be
assigned numbers instead of names.

Example:
```YAML:
system:
  dasd *:
    online: 0
```

This example specifies that all available DASDs in offline state are required.
The test program can determine the ID of each DASD during runtime using the
following environment variables:

```
TELA_SYSTEM_DASD_0
TELA_SYSTEM_DASD_1
...
```

Notes:
  - In case no object matches a wildcard resource requirement, no environment
    variable will be generated, but the test will still run.
  - Named resource requirements are served first. Only resources that are
    unassigned after fulfilling all named resource requests will be considered
    for wildcard requirements.

### Resource state

Test programs must ensure that all resources they use are returned to the
state they were in before the test program was run.

### Multiple systems

Test authors can specify that a test program requires one or more additional
systems to perform its function. This can be achieved by listing multiple
'system' entries in the test requirements file, each with a unique name and
optional additional requirements. Note that the name 'localhost' is reserved
for the local system.

Example test requirements file:
```YAML
system hosta:
  tools:
    ping:
system hostb:
```

This example defines that the test requires two systems in addition to the local
system, and that the 'ping' tool must be available on the 'hosta' system.

At runtime, tela searches for matching test systems in the test environment
file. If a match is found, tela passes the corresponding username and hostname
data to the test program via environment variables:

```
TELA_SYSTEM_hosta_SSH_USER=testuser
TELA_SYSTEM_hosta_SSH_HOST=host1
TELA_SYSTEM_hostb_SSH_USER=testuser
TELA_SYSTEM_hostb_SSH_HOST=host2
```

Test programs can use this information to directly start commands on the remote
system using an SSH client. tela also provides a tool named 'remote' that
encapsulates the process of connecting to the remote host (see next section).

### Remote tool

Test programs can make use of the 'remote' tool to perform commands on a
remote host. This tool provides the following support functions:

  - Perform commands on a remote host, either specified on command line or via
    standard input stream.
  - Create a temporary directory on the remote host - this directory is
    automatically set as current working directory for remote commands.
  - Provide an option to copy local files to the remote system before starting.
  - Provide an option to retrieve remote files after the command has finished.
  - Provide access to the console of a remote system.

To start the tool, test programs should consult the TELA\_REMOTE environment
variable to find its absolute tool path:

```
$TELA_REMOTE <options>
```


#### Usage information

```
Usage: remote [OPTIONS] <system> <command>|-

Run COMMAND on remote system identified by SYSTEM. A temporary directory
is automatically created on the remote system and used as current working
directory for the command. If "-" is specified instead of COMMAND,
read commands from standard input.

Additional options can be specified to transfer files to the remote system
before running the command, and to retrieve files from the remote system
after the command was run.

Note: This tool relies on environment variables that are set by the tela
      framework.

OPTIONS
  -l <file/directory>  Local file or directory to copy to remote
  -r <file/directory>  Remote file or directory to copy from remote
  -o <directory>       Local target directory for copied files (default: .)
  -c                   Open a console connection (precludes other options)
  -H                   Run COMMAND on hypervisor's host of a system identified
                       by SYSTEM. SYSTEM is either a name of a remote system
                       or 'localhost'.
```

#### Console interaction

Test authors can use the '-c' option of the 'remote' tool to send input to the
console of a remote system, and to receive console output. To send a single
line of input, specify the line as the COMMAND parameter. To start an
interactive console session, specify COMMAND as '-'.

In an interactive console session, all data received on the standard input
stream is sent to the console as input, and all console output is written to the
standard output stream. The console connection is closed when EOF is received
on standard input, when the process is killed by a signal, or when the console
host terminates the connection.

The 'remote' tool provides a set of internal commands to support a programmed
dialogue with the remote console. To use these commands, specify them as console
input:

  - `#tela expect <expr>`

    Wait until an output line matching the specified expression EXPR is received
    as console output. EXPR must be a valid POSIX Extended Regular Expression.

  - `#tela idle [<n>]`

    Wait until no console output has been received for at least N seconds.
    If not specified, N is assumed to be 1 second.

  - `#tela timeout <n>`

    Specify the number of seconds after which to continue even if the condition
    for a wait operation was not met. If N is specified as 0, timeout handling
    is completely disabled. The default timeout period is 20 seconds.

The 'remote' tool provides the following exit codes:

  -  0: Command completed successfully
  -  1: There was a runtime error
  -  2: Error while connecting to the console host
  -  3: A timeout occurred (e.g. during a `#tela expect` command)

Note: The current support is limited to consoles of z/VM guests.


#### Examples for using the 'remote' tool

**Example 1:** Send a single ICMP ping packet from 'hosta' to 'hostb':

```
#!/bin/bash

$TELA_REMOTE hosta ping -c 1 $TELA_SYSTEM_hostb_IP || exit 1

exit 0
```

**Example 2:** Perform multiple commands on the remote system:

```
#!/bin/bash

$TELA_REMOTE hosta -<<EOF || exit 1
hostname
date
EOF

exit 0
```

**Example 3:** Use a console connection to restart the remote z/VM system using
z/VM hypervisor commands, then wait for a login prompt to appear. Stop waiting
if the login prompt does not appear after 60 seconds:

```
#!/bin/bash

$TELA_REMOTE -c hosta -<<EOF || exit 1
#tela timeout 60
#cp ipl
#tela expect login:
EOF

exit 0
```

**Example 4:** Perform multiple commands on the hypervisor's host of the remote
system:

```
#!/bin/bash

$TELA_REMOTE -H kvmhost -<<EOF || exit 1
hostname
uname -a
EOF

exit 0
```

**Example 5:** Perform multiple commands on the hypervisor's host of the local
system:

```
#!/bin/bash

$TELA_REMOTE -H localhost -<<EOF || exit 1
hostname
uname -a
EOF

exit 0
```

# Test environment file

The test environment file is a file in YAML format that specifies the list of
resources that test programs may use during a test run. This file is named
`.telarc`. Its default location is the user's home directory. You can use the
`TELA_RC` environment variable to specify a different file location when
running tests.

You can run the following command to get a sample `.telarc` file for the local
system:

```
$ make telarc > ~/.telarc
```

Warning: The resulting test environment file will list all available resources
found on a system. Be sure to remove resources that should not be used for
testing before making use of this file.

The structure of the test environment file is similar to that of a test
YAML file with the main difference that it contains actual resource IDs (such
as device numbers) where a test YAML file contains a resource name.

Example:

Test YAML file requesting two DASDs named "a" and "b" for testing:

```YAML
system:
  dasd a:
  dasd b:
```

Test environment file listing two DASD devices 0.0.1000 and 0.0.2000 that
are available for testing:

```YAML
system:
  dasd 0.0.1000:
  dasd 0.0.2000:
```

Note that the local system is automatically considered available for testing,
even if it is not explicitly listed in the test environment file.

When running test cases, tela checks if all resources specified in the
test environment file are actually available, and automatically collects their
attribute values. Any attribute value specified in the test environment file
will be overwritten by their automatically determined counterpart.

### Meta attributes

There may be information about resources that cannot be automatically
determined. An example for such information would be:

  - Is it ok to restart the system?
  - Is it ok to delete all data on a storage device?

Such information can be encoded in 'meta attributes'. The value for these
attributes must be provided in the test environment file. Test programs
can specify requirements against these attributes as with normal attributes.

Note that only meta-attributes that are documented in the corresponding
resource object documentation should be used.

### Remote systems

In addition to resources on the local system, the test environment file can also
specify that a remote system is available for testing. Such remote systems must
be reachable via SSH.

To enlist a remote system for testing, the following SSH access information
must be provided in the test environment file:

  - Username
  - Hostname or IP address

Example .telarc file:

```YAML
system testsystem:
  ssh:
    user: root
    host: vmguest1
  dasd 0.0.1000:
```

This example specifies that the account of user "testuser" on host "testhost"
is available for testing, and that DASD resource 0.0.1000 can be used on that
host.

Note: The same user/host combination may only appear once in a test
environment file.

To enable test cases that require access to the console of remote systems,
the following information must additionally be provided in the test environment
file:

  - Hostname or IP address of the console host.
    For a z/VM system, this is the address of the z/VM host.
  - Username.
    For a z/VM system, this is the z/VM guest name.
  - Passwort.
    For a z/VM system, this is the z/VM guest password.

Example .telarc file:

```YAML
system testsystem2:
  ssh:
    user: root
    host: vmguest2
  console:
    user: vmguest2
    host: vmhost
    password: secret
```

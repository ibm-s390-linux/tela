system resource
===============

A **system** resource represents a host system on which tests can run.
There is always at least one system available for use by tests: the
local system.

A system resource provides the following attributes:

```YAML
system <name>:
  arch: <cpu_arch>
  ip: <ip_address>
  hypervisor:
    type: <hypervisor_type>
    version: <hypervisor_level>
    ssh:
      user: <username>
      host: <hostname>
  cpus:
    online: <num>
  cpu-features:
    <feat1>:
    <feat2>:
    ...
  cpu-facilities:
    <facility1>:
    <facility2>:
    ...
  cpu-mf:
    counter:
      cfvn: <num>
      csvn: <num>
      auth:
        <counter-set1>:
        <counter-set2>:
        ...
    sampling:
      min_rate: <num>
      max_rate: <num>
      modes:
        <mode1>:
        ...
    ri:
  mem:
    swaptotal: <size>
    default_hugepages_total: <num>
    default_hugepages_free: <num>
    default_hugepagesize: <size>
    transparent_hugepage: <state>
    memtotal: <size>
    hugepages:
      <size>:
        total: <num>
        free: <num>
      ...
  kernel:
    version: <kernel_ver>
    config:
      <config1>: <value>
      ...
    modules:
      <module1>: <value>
  user: <username>
  os:
    id: <os_id>
    version: <os_version>
  tools:
    <toolname1>:
    <toolname2>:
    ...
  packages:
    <package1>: [<pkg1_version>]
    <package2>: [<pkg2_version>]
  ssh:
    user: <username>
    host: <hostname>
  console:
    user: <username>
    host: <hostname>
    password: <password>
  allow_restart: <flag>
```

In addition to these attributes, other resource types (such as I/O devices)
may be associated with a system. These resources are documented in separate
documents in this directory.

### Environment variables

Test programs can access actual values of system attributes using environment
variables named:

```
   TELA_SYSTEM_<path>
```

where

  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_HYPERVISOR_TYPE=zvm
TELA_SYSTEM_HYPERVISOR_VERSION=6.4.0
TELA_SYSTEM_OS_ID=ubuntu
TELA_SYSTEM_OS_VERSION=16.04
TELA_SYSTEM_USER=test
```

### Attribute requirements

This section describes the attributes that are available for system resources.
The value of these attributes can be used in test programs (see 'Environment
variables' above) and for defining test requirements (see 'Attribute
conditions' in [Test resources](../resources.md)).

  - **`system/arch:`** *(type: scalar)*

    This attribute specifies the CPU architecture, for example, "s390x".

  - **`system/ip:`** *(type: scalar)*

    The main IPv4 address of the host.

  - **`system/hypervisor/type:`**  *(type: scalar)*

    If the system is running as a hypervisor guest, this attribute specifies
    the hypervisor type.

    Valid values are:

    - zvm: IBM z/VM
    - lpar: IBM PR/SM Logical Partition
    - kvm: Linux KVM

    Example: Test requires that it runs on a z/VM guest

    ```YAML
    system:
      hypervisor:
        type: zvm
    ```

  - **`system/hypervisor/version:`**  *(type: version)*

    Hypervisor version of the hosting hypervisor.

    Note: The hypervisor version is not available for all hypervisor types.

  - **`system/hypervisor/ssh/user:`**  *(type: scalar)*

    When specified in a resource YAML file, this attribute provides the
    username to use when connecting to the hypervisor host of a remote system
    via SSH.

    When specified in a YAML requirements, this attribute indicates that a test
    requires SSH access to the hypervisor host of a remote system.

    See also the section on "Remote systems" in [Test resources](../resources.md).

  - **`system/hypervisor/ssh/host:`**  *(type: scalar)*

    When specified in a resource YAML file, this attribute provides the
    hostname to use when connecting to the hypervisor host of a remote system
    via SSH.

    When specified in a YAML requirements, this attribute indicates that a test
    requires SSH access to the hypervisor host of a remote system.

    At the moment, only hypervisors of type KVM are able to provide SSH access.

    See also the section on "Remote systems" in [Test resources](../resources.md).

    Example: Test requires that it runs on a KVM guest with SSH access
             to hypervisor's host:

    ```YAML
    system:
      hypervisor:
        type: kvm
        ssh:
          user:
          host:
    ```

  - **`system/cpus/online:`** *(type: scalar)*

    Number of CPUs that are online.  The following short form is also
    supported:
    ```YAML
    system:
      cpus: <num>
    ```

  - **`system/cpu-features/<feature>:`** *(type: scalar)*

    List features (hardware capabilities) that are supported by the CPU
    architecture.  Examples are "vx" for Vector Extension (SIMD) or "te"
    for Transactional Execution support.  The CPU feature list is obtained
    from /proc/cpuinfo.

    Example: Test requires transactional execution support:

    ```YAML
    system:
      cpu-features:
        te:
    ```

  - **`system/cpu-facilities/<facility>:`** *(type: scalar)*

    List of facility bits (in decimal notation). The list is obtained
    from /proc/cpuinfo.

  - **`system/cpu-mf:`** *(type: scalar)*

    Provides information about CPU-Measurement Facilities.  For each
    installed facility, a section is added.

  - **`system/cpu-mf/counter:`** *(type: scalar)*

    Indicates that the CPU-Measurement Counter Facility is available.
    Additional child sections contain details about the Counter Facility.

  - **`system/cpu-mf/counter/cfvn:`** *(type: number)*

    Provides the Counter First Version Number (CFVN).

  - **`system/cpu-mf/counter/csvn:`** *(type: number)*

    Provides the Counter Second Version Number (CSVN).

  - **`system/cpu-mf/counter/auth/<counter-set>:`** *(type: scalar)*

    Authorized counter sets.  Valid counter sets are "basic", "problem-state",
    "crypto-activity", "extended", and "mt-diagnostic".

  - **`system/cpu-mf/sampling:`** *(type: scalar)*

    Indicates that the CPU-Measurement Sampling Facility is available.
    Additional child sections contain details about the Sampling Facility.

  - **`system/cpu-mf/sampling/min_rate:`** *(type: number)*

    Minimum sampling interval in cycles.

  - **`system/cpu-mf/sampling/max_rate:`** *(type: number)*

    Maximum sampling interval in cycles.

  - **`system/cpu-mf/sampling/modes/<mode>:`** *(type: scalar)*

    Authorized sampling modes.  Valid sampling modes are "basic" and "diagnostic".

  - **`system/cpu-mf/ri:`** *(type: scalar)*

    Indicates that the Runtime Instrumentation Facility is available.

  - **`system/mem:``**

    Memory information about the system.  You can use this attribute to
    specify the required amount of (physical) memory for a test case.

    Example: Test requires at least 4GiB memory:

    ```YAML
    system:
      mem: >=4GiB
    ```

  - **`system/mem/memtotal`** *(type: number)*

    Physical memory of the system.  Test requirements can also use the
    abbreviated form with `system/mem:` (see above).

  - **`system/mem/swaptotal:`** *(type: number)*

    Size in bytes of total swap.

  - **`system/mem/transparent_hugepage:`** *(type: scalar)*

    Enablement status of transparent huge pages.  Values are
    obtained from sysfs.  Valid values are: always, madvice,
    or never.

  - **`system/mem/default_hugepagesize:`** *(type: number)*

    Default huge page size in bytes.

  - **`system/mem/default_hugepages_total:`** *(type: scalar)*

    Total number of huge pages with the default huge page size.

  - **`system/mem/default_hugepages_free:`** *(type: scalar)*

    Number of free huge pages with the default huge page size.

  - **`system/mem/hugepages/<size>/total:`** *(type: scalar)*

    Total number of huge pages of the given size, <size>.
    The <size> designates the size of the huge page.  This might
    not be the default size.

  - **`system/mem/hugepages/<size>/free:`** *(type: scalar)*

    Number of free huge pages of the given huge page size, <size>.
    The <size> designates the size of the huge page.  This might
    not be the default size.

  - **`system/kernel:`**

    Information about the Linux kernel.  You can use this attribute
    to specify the kernel release version, for example, "4.19".

  - **`system/kernel/version:`** *(type: version)*

    This attribute specifies the kernel release version, e.g. "4.14".

  - **`system/kernel/config/<config>:`** *(type: scalar)*

    When specified in a test YAML file, this attribute indicates that the
    kernel configuration option, `<config>`, must be set. Note that the
    CONFIG_ prefix is omitted for `<config>`.  Further requirements can be
    specified as values.

    Valid values are:

    - y: for kernel modules or functions that are built into the kernel.
    - m: for functionality that is built as loadable module.
    - n: for kernel modules or functions that are not set
    - <value>: value of kernel configuration options, for example, the
      number of CPUs.

    The sample test YAML file requires the CONFIG_BLOCK, CONFIG_NR_CPUS,
    and CONFIG_ISO9660_FS kernel configuration options.
    ```YAML
    system:
      kernel:
        config:
          BLOCK:
          NR_CPUS:
          ISO9660_FS:
    ```

  - **`system/kernel/modules/<module>:`** *(type: scalar)*

    When specified in a test YAML file, this attribute indicates that the
    kernel module, `<module>`, must be present. If a module entry is
    present, the respective environment variable contains the path
    to the kernel module or **built-in** (if the module is built into
    the kernel).

    The sample test YAML file requires the kvm module.
    ```YAML
    system:
      kernel:
        modules:
          kvm:
    ```

  - **`system/user:`**  *(type: scalar)*

    Username of the account that is used to run test programs on the system
    resource.

    Example: Test requires that user is not `root`
    ```YAML
    system:
      user: != root
    ```

  - **`system/os/id:`**  *(type: scalar)*

    Operating system ID of the OS that is running on the system resource.

    Note: To find out the corresponding ID value for a specific OS, run the
    helper program found at `src/libexec/os` in the tela repository.

  - **`system/os/version:`**  *(type: version)*

    Operating system version of the OS that is running on the system resource.

  - **`system/tools/<name>:`**  *(type: scalar)*

    When specified in a test YAML file, this attribute indicates that the
    executable with the specified `<name>` must be available in the program
    search path (PATH environment variable) on the system resource.

    Example test YAML: Test requires the `lsblk` tool:
    ```YAML
    system:
      tools:
        lsblk:
    ```

    The associated environment variable `TELA_SYSTEM_TOOLS_lsblk` contains the
    absolute path to the executable.

    Test programs should use this environment variable when referring to the
    executable. This way, tela users can override the path to a specific binary
    used in test programs by specifying a value for the corresponding tools
    attribute in the resource YAML file.

    Example resource YAML: Use `/home/user/perf` instead perf's system install
    path:
    ```YAML
    system:
      tools:
        perf: /home/user/perf
    ```

  - **`system/packages/<name>:`**  *(type: version)*

    When specified in a test YAML file, this attribute indicates that the
    package with the specified `<name>` must be installed on the system
    resource. An optional version condition can be specified to require a
    specific package version.

    Example: Test requires that Bash (any version) and Perl (version 5.10 and
    above) are installed:
    ```YAML
    system:
      packages:
        bash:
        perl: >= 5.10
    ```

    Test programs can query actual package versions during runtime via the
    corresponding environment variables:

    ```
    TELA_SYSTEM_PACKAGES_bash=4.3-14ubuntu1.2
    TELA_SYSTEM_PACKAGES_perl=5.22.1-9
    ```

  - **`system/ssh/user:`**  *(type: scalar)*

    When specified in a resource YAML file, this attribute provides the
    username to use when connecting to a remote system via SSH.

    See also the section on "Remote systems" in [Test resources](../resources.md).

  - **`system/ssh/host:`**  *(type: scalar)*

    When specified in a resource YAML file, this attribute provides the
    hostname to use when connecting to a remote system via SSH.

    See also the section on "Remote systems" in [Test resources](../resources.md).

  - **`system/console/user:`**  *(type: scalar)*

    When specified in a resource YAML file, this attribute provides the
    username to use when connecting to the console of a remote system.

    See also the section on "Remote systems" in [Test resources](../resources.md).

    Note: A test case that wants to connect to the console of a remote system
    must specify the 'console' attribute as a requirement in its requirement
    YAML file.

    Example: Test requires access to remote console:

    ```yaml
    system sys1:
      console:
    ```

  - **`system/console/host:`**  *(type: scalar)*

    When specified in a resource YAML file, this attribute provides the
    hostname to use when connecting to the console of a remote system.

    See also the section on "Remote systems" in [Test resources](../resources.md).

  - **`system/console/password:`**  *(type: scalar)*

    When specified in a resource YAML file, this attribute provides the
    password to use when connecting to the console of a remote system.

    See also the section on "Remote systems" in [Test resources](../resources.md).

  - **`system/allow_restart:`**  *(type: scalar)*

    This attribute indicates whether a test case is allowed to restart a remote
    system. Valid values are:

      - 0: Test case must not cause the remote system to restart (default)
      - 1: Test case is allowed to restart the remote system

    Note that even with a value of 1, the operational state of the remote system
    at the end of the test must be the same as at the start of the test case.

    Example: Test requires a remote system that can be restarted

    ```YAML
    system sys1:
      allow_restart: 1
    ```

    Note: A non-default value for this attribute must be specified manually
    in the test environment file as it cannot be determined automatically.

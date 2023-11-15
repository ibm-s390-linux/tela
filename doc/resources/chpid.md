chpid resource
==============

A **chpid** resource represents a Channel-Subsystem Channel Path. It provides
the following attributes:

```YAML
system:
  chpid:
    busid: <busid>
    sysfs: <sysfs_path>
    online: <online_state>
    configured: <configured_state>
    type: <chpid_type>
    chid: <chid>
    chid_external: <flag>
    pnetid:
      id: <pnetid>
    pnetid_count: <count>
    allow_offline: <flag>
```
Additionally the following system attributes related to **chpid** resources
are provided:
```YAML
system:
  chpid_count_available: <number>
  chpid_count_total: <number>
```

### Environment variables

Test programs can access actual values of chpid attributes using environment
variables named:

```
   TELA_SYSTEM_CHPID_<name>_<path>
```

where

  - `<name>` is the symbolic name given to a chpid in the test YAML file
  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_CHPID_COUNT_AVAILABLE=99
TELA_SYSTEM_CHPID_COUNT_TOTAL=99
TELA_SYSTEM_CHPID_mychpid=0.30
TELA_SYSTEM_CHPID_mychpid_ALLOW_OFFLINE=0
TELA_SYSTEM_CHPID_mychpid_BUSID=0.30
TELA_SYSTEM_CHPID_mychpid_CHID=0x0105
TELA_SYSTEM_CHPID_mychpid_CHID_EXTERNAL=1
TELA_SYSTEM_CHPID_mychpid_CONFIGURED=1
TELA_SYSTEM_CHPID_mychpid_ONLINE=1
TELA_SYSTEM_CHPID_mychpid_PNETID_a=0
TELA_SYSTEM_CHPID_mychpid_PNETID_a_ID=NET0
TELA_SYSTEM_CHPID_mychpid_PNETID_COUNT=1
TELA_SYSTEM_CHPID_mychpid_SYSFS=/sys/devices/css0/chp0.30
TELA_SYSTEM_CHPID_mychpid_TYPE=0x1b
```

### Attribute description

This section describes the attributes that are available for chpid resources.
The value of these attributes can be used in test programs (see 'Environment
variables' above) and for defining test requirements (see 'Attribute
conditions' in [Test resources](../resources.md)).

  - **`system/chpid/busid:`**  *(type: scalar)*

    The bus-ID of the Channel Path.

  - **`system/chpid/sysfs:`**  *(type: scalar)*

    Sysfs-path associated with the Channel Path.

  - **`system/chpid/online:`**  *(type: scalar)*

    Logical online state of the Channel Path. Valid values are 0 for 'offline'
    and 1 for 'online'.

  - **`system/chpid/configured:`**  *(type: scalar)*

    Configuration state of the Channel Path. Valid values are 0 for 'standby'
    and 1 for 'configured'.

  - **`system/chpid/type:`**  *(type: number)*

    Numerical Channel Path type.

  - **`system/chpid/chid:`**  *(type: scalar)*

    The Channel ID that is associated with this Channel Path. This ID is either
    a physical channel ID (PCHID) or an internal Channel ID.

    This property does not necessarily exist in each (old) Linux kernel version.
    If it's missing, .yaml resource requirements for chid cannot be fulfilled.
    In that case, you can manually add the missing parts to your test
    environment local telarc file:
    ```YAML
    system:
      chpid 0.5a:
        chid: 11c
    ```

    See also attribute `chid_external`.

  - **`system/chpid/chid_external:`**  *(type: scalar)*

    A flag that indicates whether this Channel Path is associated with an
    internal channel ID (value 0) or a physical channel ID (value 1).

  - **`system/chpid/pnetid:`**  *(type: object)*

    A physical network ID that is defined for a port of the Channel Path.
    One Channel Path can have up to 4 PNETIDs.

  - **`system/chpid/pnetid/id:`**  *(type: scalar)*

    The actual symbolic PNETID.

    Example: Test requires two Channel Paths that are attached to the same
    physical network.

    ```YAML
    system:
      chpid chpid1:
        pnetid a:
          id: %{pnetid}
      chpid chpid2:
        pnetid a:
          id: %{pnetid}
    ```

    Note: By listing a CHPID in the resource YAML file, all associated
    PNETIDs are automatically registered as available for testing.

  - **`system/chpid/pnetid_count:`** *(type: number)*

    Total number of PNETIDs that are defined for a Channel Path.

  - **`system/chpid/allow_offline:`**  *(type: scalar)*

    This attribute indicates whether a test case is allowed to vary a Channel
    Path logically offline, or to configure it into the standby state. Valid
    values are:

      - 0: Test case must not perform vary off or configure standby (default)
      - 1: Test case is allowed to vary or configure Channel Path offline

    Note: A non-default value for this attribute must be specified manually
    in the test environment file as it cannot be determined automatically.

  - **`system/chpid_count_available:`** *(type: number)*

    Number of chpid resource objects that are available for test cases.

  - **`system/chpid_count_total:`** *(type: number)*

    Total number of chpid resource objects that are defined on the
    test system.

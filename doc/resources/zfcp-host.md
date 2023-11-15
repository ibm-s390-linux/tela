zfcp-host resource
=============

A tela **zfcp-host** resource represents an FCP device. It provides the
following attributes:

```YAML
system:
  zfcp-host:
    busid: <busid>
    sysfs: <sysfs_path>
    allow_inject: 0
    chpid:
      <chpid_data>
    online: <online_state>
    card_version: <card_version>
    lic_version: <lic_version>
    datarouter: <datarouter_state>
    fc_host:
      port_name: <wwpn>
      permanent_port_name: <wwpn>
      port_type: <type>
      port_id: <n_port_id>
      serial_number: <serial>
      speed: <speed_in_bps>
    scsi_host:
      prot_capabilities: <integrity_type>
      prot_guard_type: <guard_type>
```

Additionally the following system attributes related to **zfcp-host** resources
are provided:
```YAML
system:
  zfcp_host_count_available: <number>
  zfcp_host_count_total: <number>
```

### Environment variables

Test programs can access actual values of zfcp-host attributes using
environment variables named:

```
   TELA_SYSTEM_ZFCP_HOST_<name>_<path>
```

where

  - `<name>` is the symbolic name given to an zfcp-host resource in the test
     YAML file
  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_ZFCP_HOST_COUNT_AVAILABLE=1
TELA_SYSTEM_ZFCP_HOST_COUNT_TOTAL=2
TELA_SYSTEM_ZFCP_HOST_host_a=0.0.190d
TELA_SYSTEM_ZFCP_HOST_host_a_ALLOW_INJECT=0
TELA_SYSTEM_ZFCP_HOST_host_a_BUSID=0.0.190d
TELA_SYSTEM_ZFCP_HOST_host_a_CARD_VERSION=0x0008
TELA_SYSTEM_ZFCP_HOST_host_a_CHPID_chpid_a=0.60
TELA_SYSTEM_ZFCP_HOST_host_a_CHPID_chpid_a_ALLOW_OFFLINE=0
TELA_SYSTEM_ZFCP_HOST_host_a_CHPID_chpid_a_BUSID=0.60
TELA_SYSTEM_ZFCP_HOST_host_a_CHPID_chpid_a_CHID=0x0120
TELA_SYSTEM_ZFCP_HOST_host_a_CHPID_chpid_a_CHID_EXTERNAL=1
TELA_SYSTEM_ZFCP_HOST_host_a_CHPID_chpid_a_CONFIGURED=1
TELA_SYSTEM_ZFCP_HOST_host_a_CHPID_chpid_a_ONLINE=1
TELA_SYSTEM_ZFCP_HOST_host_a_CHPID_chpid_a_SYSFS=/sys/devices/css0/chp0.60
TELA_SYSTEM_ZFCP_HOST_host_a_CHPID_chpid_a_TYPE=0x25
TELA_SYSTEM_ZFCP_HOST_host_a_DATAROUTER=1
TELA_SYSTEM_ZFCP_HOST_host_a_FC_HOST_PERMANENT_PORT_NAME=0xc05076ffeb001201
TELA_SYSTEM_ZFCP_HOST_host_a_FC_HOST_PORT_ID=0x67e34a
TELA_SYSTEM_ZFCP_HOST_host_a_FC_HOST_PORT_NAME=0xc05076ffeb001b1c
TELA_SYSTEM_ZFCP_HOST_host_a_FC_HOST_PORT_TYPE='NPIV VPORT'
TELA_SYSTEM_ZFCP_HOST_host_a_FC_HOST_SERIAL_NUMBER=IBM020000000829E7
TELA_SYSTEM_ZFCP_HOST_host_a_FC_HOST_SPEED=16000000000
TELA_SYSTEM_ZFCP_HOST_host_a_LIC_VERSION=0x10300142
TELA_SYSTEM_ZFCP_HOST_host_a_ONLINE=1
TELA_SYSTEM_ZFCP_HOST_host_a_SCSI_HOST_PROT_CAPABILITIES=0
TELA_SYSTEM_ZFCP_HOST_host_a_SCSI_HOST_PROT_GUARD_TYPE=0
TELA_SYSTEM_ZFCP_HOST_host_a_SYSFS=/sys/bus/ccw/drivers/zfcp/0.0.190d
```

### Attribute description

This section describes the attributes that are available for zfcp-host
resources. The value of these attributes can be evaluated in test programs
(see 'Environment variables' above) and for defining test requirements
(see 'Attribute conditions' in [Test resources](../resources.md)).

  - **`system/zfcp-host/busid:`** *(type: scalar)*

    The bus-ID of the FCP device. For example '0.0.190d'.

  - **`system/zfcp-host/sysfs:`** *(type: scalar)*

    Sysfs-path associated with the FCP device.

  - **`system/zfcp-host/allow_inject:`** *(type: scalar)*

    This attribute indicates whether a test case is allowed to perform injects
    on the FCP device such as pulling the cable. Valid values are:

      - 0: Test case must not perform injects (default)
      - 1: Test case is allowed to perform injects such as pulling the cable

    Example: Test requires an FCP device for which the cable may be pulled

    ```YAML
    system:
      zfcp-host a:
        allow_inject: 1
    ```

    Note: A non-default value for this attribute must be specified manually
    in the test environment file as it cannot be determined automatically.

  - **`system/zfcp-host/chpid:`** *(type: object)*

    A CHPID that is defined for the FCP device. See [chpid resource](chpid.md)
    for a list of available chpid resource attributes.

    Example: Test requires two separate FCP devices on different physical
    FCP channels.

    ```YAML
    system:
      zfcp-host host_a:
        chpid chpid_a:
          chid: %{id}
      zfcp-host host_b:
        chpid chpid_b:
          chid: != %{id}
    ```

    Note: By listing an FCP device in the resource YAML file, the associated
    CHPID is automatically registered as available for testing. Also note that
    to override attribute values of an FCP device's CHPID in a resource YAML
    file, the attributes must be specified in the corresponding CHPID section
    (system/chpid) and not in the FCP device's CHPID section
    (system/zfcp-host/chpid).

    The chid property of chpid does not necessarily exist in each (old) Linux
    kernel version. If it's missing, .yaml resource requirements for chid cannot
    be fulfilled. In that case, you can manually add the missing parts to your
    test environment local telarc file:
    ```YAML
    system:
      chpid 0.5a:
        chid: 11c
    ```

  - **`system/zfcp-host/online:`** *(type: scalar)*

    Online state of the FCP device. Valid values are 0 for 'offline' and 1 for
    'online'. Note that if a test YAML file does not contain a condition for
    the online attribute, both online and offline FCP devices will match.

    Example: Test requires an online FCP device

    ```YAML
    system:
      zfcp-host my_zfcp_host:
        online: 1
    ```

  - **`system/zfcp-host/card_version:`** *(type: number)*

    Version number of the FCP hardware. For example '0x0008'.

  - **`system/zfcp-host/lic_version:`** *(type: number)*

    Microcode level of the FCP hardware. For example '0x10300142'.

  - **`system/zfcp-host/datarouter:`** *(type: scalar)*

    This attribute indicates whether the data router hardware feature is
    active at the FCP device. Valid values are:

    - 0: Inactive
    - 1: Active

  - **`system/zfcp-host/fc_host/port_name:`** *(type: scalar)*

    WWPN associated with the FCP device. If NPIV is not active, this is the
    same as the 'permanent_port_name'. Active NPIV support is reflected in
    the corresponding 'port_type' attribute.

  - **`system/zfcp-host/fc_host/permanent_port_name:`** *(type: scalar)*

    WWPN associated with the physical port of the FCP channel.

  - **`system/zfcp-host/fc_host/port_type:`** *(type: scalar)*

    Textual ID of the port topology. When operating in NPIV mode, the value
    is 'NPIV VPORT'.

  - **`system/zfcp-host/fc_host/port_id:`** *(type: scalar)*

    The N\_Port\_ID assigned to the port by the fabric.

  - **`system/zfcp-host/fc_host/serial_number:`** *(type: scalar)*

    Serial number of the adapter hardware that provides the FCP channel.

  - **`system/zfcp-host/fc_host/speed:`** *(type: number)*

    Speed of the FC link in bits per second (bps).

    Example: Test requires an FCP device associated with an FCP channel that
    operates with at least 16 Gbps

    ```YAML
    system:
      zfcp-host a:
        fc_host:
          speed: >= 16G
    ```

  - **`system/zfcp-host/scsi_host/prot_capabilities:`** *(type: number)*

    Level of end-to-end data consistency checking available at FCP device.
    This is based on FCP device capabilities and zfcp kernel module settings.
    Valid values are:

    - 0: No checking
    - 1: DIF type 1
    - 16: DIX type 1
    - 17: DIX type 1 with DIF type 1

  - **`system/zfcp-host/scsi_host/prot_guard_type:`** *(type: number)*

    Supported checksum types. Valid values are:

    - 1: CRC checksum
    - 2: IP checksum

  - **`system/zfcp_host_count_available:`** *(type: number)*

    Number of zfcp-host resource objects that are available for test cases.

  - **`system/zfcp_host_count_total:`** *(type: number)*

    Total number of zfcp-host resource objects that are defined on the
    test system.

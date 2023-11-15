qeth resource
============

A tela **qeth** resource represents a network device provided by the QETH device
driver. This includes real and simulated Open Systems Adapters (OSA) and
Hipersockets devices. A qeth resource is identified by its CCW-group device ID,
e.g. '0.0.f500'.

The following attributes are provided for qeth resources:

```YAML
system:
  qeth:
    busid: <busid>
    sysfs: <sysfs>
    chpid:
      <chpid_data>
    online: <online_state>
    layer2: <layer2_setting>
    card_type: <type>
    portno: <port_number>
    netdev:
      if_name: <if_name>
      speed: <line_speed>
```
Additionally the following **system** attributes related to qeth resources
are provided:
```YAML
system:
  qeth_count_available: <number>
  qeth_count_total: <number>
```

### Environment variables

Test programs can access actual values of qeth attributes using environment
variables named:

```
   TELA_SYSTEM_QETH_<name>_<path>
```

where

  - `<name>` is the symbolic name given to a qeth resource in the test YAML file
  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_QETH_COUNT_AVAILABLE=2
TELA_SYSTEM_QETH_COUNT_TOTAL=2
TELA_SYSTEM_QETH_myqeth=0.0.f500
TELA_SYSTEM_QETH_myqeth_BUSID=0.0.f500
TELA_SYSTEM_QETH_myqeth_CARD_TYPE=OSD_1000
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid=0.94
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_ALLOW_OFFLINE=0
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_BUSID=0.94
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_CHID=0x0104
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_CHID_EXTERNAL=1
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_CONFIGURED=1
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_ONLINE=1
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_PNETID_COUNT=1
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_PNETID_mypnetid=0
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_PNETID_mypnetid_ID=NET0
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_SYSFS=/sys/devices/css0/chp0.94
TELA_SYSTEM_QETH_myqeth_CHPID_mychpid_TYPE=0x11
TELA_SYSTEM_QETH_myqeth_LAYER2=1
TELA_SYSTEM_QETH_myqeth_NETDEV_IF_NAME=enccw0.0.f500
TELA_SYSTEM_QETH_myqeth_NETDEV_SPEED=1000
TELA_SYSTEM_QETH_myqeth_ONLINE=1
TELA_SYSTEM_QETH_myqeth_PORTNO=0
TELA_SYSTEM_QETH_myqeth_SYSFS=/sys/bus/ccwgroup/drivers/qeth/0.0.f500
```

### Attribute description

This section describes the attributes that are available for qeth resources.
The value of these attributes can be used in test programs (see 'Environment
variables' above) and for defining test requirements (see 'Attribute
conditions' in [Test resources](../resources.md)).

  - **`system/qeth/busid:`**  *(type: scalar)*

    The bus-ID of the QETH device.

  - **`system/qeth/sysfs:`**  *(type: scalar)*

    Sysfs-path associated with the QETH device.

  - **`system/qeth/chpid:`** *(type: object)*

    A CHPID that is defined for the QETH device. See [chpid resource](chpid.md)
    for a list of available chpid resource attributes.

    Example: Test requires a QETH with a CHPID that can be varied offline.

    ```YAML
    system:
      qeth myqeth:
        chpid mychpid:
          allow_offline: 1
    ```

    Note: By listing a QETH device in the resource YAML file, all associated
    CHPIDs are automatically registered as available for testing. Also note that
    to override attribute values of a QETH device's CHPID in a resource YAML
    file, the attributes must be specified in the corresponding CHPID section
    (system/chpid) and not in the QETH device's CHPID section
    (system/qeth/chpid).

  - **`system/qeth/online:`**  *(type: scalar)*

    Online state of the QETH device. Valid values are 0 for 'offline' and 1 for
    'online'. Note that if a test YAML file does not contain a condition for
    the online attribute, both online and offline QETH devices will match.

    Example: Test requires an online QETH device

    ```YAML
    system:
      qeth myqeth:
        online: 1
    ```

  - **`system/qeth/layer2:`**  *(type: number)*

    Layer mode of the QETH device. Valid values are 0 for layer 3 mode and 1
    for layer 2 mode.

  - **`system/qeth/card_type:`**  *(type: scalar)*

    QETH device type information.

  - **`system/qeth/portno:`**  *(type: number)*

    Relative port number of the QETH device.

  - **`system/qeth/netdev/if_name:`**  *(type: scalar)*

    Name of the Linux networking interface that is provided by the QETH device,
    e.g. 'eth0'.

  - **`system/qeth/netdev/speed:`**  *(type: number)*

    The speed in Mbits/sec at which the networking interface is operating.

  - **`system/qeth_count_available:`** *(type: number)*

    Number of QETH devices that are available for test cases.

  - **`system/qeth_count_total:`** *(type: number)*

    Total number of QETH devices that are defined on the test system.

    Example: Test requires a system with no QETH device.

    ```YAML
    system:
      qeth_count_total: 0
    ```

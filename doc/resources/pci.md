pci resource
============

A tela **pci** resource represents a PCI function. It is identified by the PCI
function ID associated with the PCI function, for example '0x00000850'.

The following attributes are provided for pci resources:

```YAML
system:
  pci <function_id>:
    busid: <busid>
    class: <class_code>
    device: <device_id>
    fid: <function_id>
    netdev:
      if_name: <if_name>
      pport: <physical_port>
      pnetid: <pnetid>
    pchid: <physical_channel_id>
    port: <physical_channel_port>
    pft: <pci_function_type>
    pnetid:
      id: <pnetid>
    pnetid_count: <count>
    sysfs: <sysfs>
    uid: <pci_uid>
    uid_is_unique: 0|1
    vendor: <vendor_id>
    vfn: <virtual_fn_number>
```
Additionally the following **system** attributes related to pci resources
are provided:
```YAML
system:
  pci_count_available: <number>
  pci_count_total: <number>
```

Note: PCI functions can be specified in different ID formats in a telarc file.

Either by PCI function ID:

```YAML
pci <function_id>:
pci fid:<function_id>:
```

or by the PCI User-Defined ID (UID):

```YAML
pci uid:<pci_uid>:
```

### Environment variables

Test programs can access actual values of pci attributes using environment
variables named:

```
   TELA_SYSTEM_PCI_<name>_<path>
```

where

  - `<name>` is the symbolic name given to a pci resource in the test YAML file
  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_PCI_COUNT_AVAILABLE=1
TELA_SYSTEM_PCI_COUNT_TOTAL=2
TELA_SYSTEM_PCI_mypci=0x00000224
TELA_SYSTEM_PCI_mypci_BUSID=0000:00:00.0
TELA_SYSTEM_PCI_mypci_CLASS=0x020000
TELA_SYSTEM_PCI_mypci_DEVICE=0x1016
TELA_SYSTEM_PCI_mypci_FID=0x00000224
TELA_SYSTEM_PCI_mypci_NETDEV_ndev=0
TELA_SYSTEM_PCI_mypci_NETDEV_ndev_IF_NAME=eno2368
TELA_SYSTEM_PCI_mypci_NETDEV_ndev_PPORT=0
TELA_SYSTEM_PCI_mypci_NETDEV_ndev_PNETID=NET1
TELA_SYSTEM_PCI_mypci_PCHID=0x01fc
TELA_SYSTEM_PCI_mypci_PORT=2
TELA_SYSTEM_PCI_mypci_PFT=0x0a
TELA_SYSTEM_PCI_mypci_PNETID_a=0
TELA_SYSTEM_PCI_mypci_PNETID_a_ID=NET1
TELA_SYSTEM_PCI_mypci_PNETID_COUNT=2
TELA_SYSTEM_PCI_mypci_SYSFS=/sys/bus/pci/devices/0000:00:00.0
TELA_SYSTEM_PCI_mypci_UID=0x0
TELA_SYSTEM_PCI_mypci_VENDOR=0x15b3
TELA_SYSTEM_PCI_mypci_VFN=0x000a
```

### Attribute description

This section describes the attributes that are available for pci resources.
The value of these attributes can be used in test programs (see 'Environment
variables' above) and for defining test requirements (see 'Attribute
conditions' in [Test resources](../resources.md)).

  - **`system/pci/busid:`**  *(type: scalar)*

    The bus-ID of the PCI function.

  - **`system/pci/class:`**  *(type: number)*

    PCI device class code, consisting of Class Code, Sub-class, and programming
    interface ID (Prog IF).

    Example: Test requires a PCI function that implements an ethernet networking
    device:

    ```YAML
    system:
      pci mypci:
        class: 0x020000
    ```

  - **`system/pci/device:`**  *(type: number)*

    PCI device ID.

  - **`system/pci/fid:`**  *(type: number)*

    Function identifier of the PCI function.

  - **`system/pci/netdev:`**  *(type: object)*

    A Linux networking interface that is provided by the PCI device.
    One PCI function can have up to 2 network devices (with RoCE CX3 adapters).

  - **`system/pci/netdev/if_name:`**  *(type: scalar)*

    Name of the Linux networking interface that is provided by the PCI device,
    e.g. 'eno2368'.

  - **`system/pci/netdev/pport:`**  *(type: number)*

    The physical port of the RoCE Express adapter for this network device.
    Might be 0 or 1 and identifies the external Eth port.

    For VFs on RoCE Express 2/3 this is always 0.
    Please use `system/pci/port` to distinguish the ports, instead.

  - **`system/pci/netdev/pnetid:`**  *(type: scalar)*

    The PNETID of the physical port.

  - **`system/pci/pchid:`**  *(type: number)*

    The physical channel ID associated with the PCI function.

  - **`system/pci/pft:`**  *(type: number)*

    PCI function type. Example types:

    - 2: RoCE Express
    - 3: zEDC
    - 5: ISM
    - 7: zHyperlink (Sync IO)
    - 8: Regional Crypto Enablement
    - 10: RoCE Express 2/3
    - 11: NVMe

  - **`system/pci/pnetid:`**  *(type: object)*

    A physical network ID that is defined for a port of the PCI function.
    One PCI function can have up to 4 PNETIDs.

  - **`system/pci/pnetid/id:`**  *(type: scalar)*

    The actual symbolic PNETID.

    Example: Test requires two PCI functions that are attached to the same
    physical network.

    ```YAML
    system:
      pci pci1:
        pnetid a:
          id: %{pnetid}
      pci pci2:
        pnetid a:
          id: %{pnetid}
    ```

    Note: By listing a PCI function in the resource YAML file, all associated
    PNETIDs are automatically registered as available for testing.

  - **`system/pci/pnetid_count:`** *(type: number)*

    Total number of PNETIDs that are defined for a PCI function.

  - **`system/pci/port:`**  *(type: number)*

    The external port as reported by the PCI layer.

    * For RoCE Express 2/3 values 1 and 2 represent Eth port 1 and 2,
      respectively.
    * For RoCE Express 1 this is 0 - and the Eth ports are distinguished
      by `system/pci/netdev/pport`.
    * For NVMe (and other functions with external ports) this is always 0.

  - **`system/pci/sysfs:`**  *(type: scalar)*

    Sysfs-path associated with the PCI function.

  - **`system/pci/uid:`**  *(type: number)*

    User-defined identifier for this function.

  - **`system/pci/uid_is_unique:`**  *(type: scalar)*

    Flag that specifies if the user-defined identifier (UID) for this function
    is ensured to be unique within this system.

      - 0: UID is not unique
      - 1: UID is unique
      - -: Flag is not available

  - **`system/pci/vendor:`**  *(type: number)*

    PCI vendor ID.

  - **`system/pci/vfn:`**  *(type: number)*

    Virtual function number for virtual PCI functions, or 0 for physical
    PCI functions.

    Example: Test requires a physical PCI function:

    ```YAML
    system:
      pci mypci:
        vfn: 0
    ```

  - **`system/pci_count_available:`** *(type: number)*

    Number of PCI functions that are available for test cases.

  - **`system/pci_count_total:`** *(type: number)*

    Total number of PCI functions that are defined on the test system.

    Example: Test requires a system with no PCI function.

    ```YAML
    system:
      pci_count_total: 0
    ```

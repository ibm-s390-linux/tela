iscsi-lun resource
==================

A tela **iscsi-lun** resource represents a SCSI device
attached through a software-based iSCSI initiator.
It provides the following attributes:

```YAML
system:
  iscsi-lun:
    id: <devid>
    ipaddress: <IP_address>
    port: <port>
    target: <iSCSI_target_name>
    iscsilun: <iSCSI_LUN>
    allow_write: <flag>
    scsi_dev:
      sysfs: <scsi_sysfs_path>
      hctl: <scsi_device_name>
      host: <scsi_host>
      channel: <scsi_channel>
      target: <scsi_target>
      lun: <scsi_lun>
      wwid: <wwid>
      vendor: <vendor>
      model: <model>
      type: <peripheral_device_type>
      scsi_disk:
        protection_type: <t10_dif_type>
        thin_provisioning: <thin_or_resource>
    block_dev:
      bdev: <blockdev_name>
      size: <blockdev_size>
      blksize: <blocksize>
```

Additionally the following system attributes related to **iscsi-lun** resources
are provided:
```YAML
system:
  iscsi_lun_count_available: <number>
  iscsi_lun_count_total: <number>
```

### Environment variables

Test programs can access actual values of iscsi-lun attributes using
environment variables named:

```
   TELA_SYSTEM_ISCSI_LUN_<name>_<path>
```

where

  - `<name>` is the symbolic name given to an iscsi-lun resource in the
    test YAML file
  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_ISCSI_LUN_COUNT_AVAILABLE=1
TELA_SYSTEM_ISCSI_LUN_COUNT_TOTAL=8
TELA_SYSTEM_ISCSI_LUN_mylun=ip-10.210.9.1:3260-iscsi-iqn.1986-03.com.ibm:2145.v7k06.node1-lun-12
TELA_SYSTEM_ISCSI_LUN_mylun_ID=ip-10.210.9.1:3260-iscsi-iqn.1986-03.com.ibm:2145.v7k06.node1-lun-12
TELA_SYSTEM_ISCSI_LUN_mylun_IPADDRESS=10.210.9.1
TELA_SYSTEM_ISCSI_LUN_mylun_PORT=3260
TELA_SYSTEM_ISCSI_LUN_mylun_TARGET=iqn.1986-03.com.ibm:2145.v7k06.node1
TELA_SYSTEM_ISCSI_LUN_mylun_ISCSILUN=12
TELA_SYSTEM_ISCSI_LUN_mylun_ALLOW_WRITE=0
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_CHANNEL=0
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_HOST=0
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_LUN=12
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_MODEL=2145
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_HCTL=0:0:0:12
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_SYSFS=/sys/bus/scsi/devices/0:0:0:12/
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_TARGET=0
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_TYPE=1
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_VENDOR=IBM
TELA_SYSTEM_ISCSI_LUN_mylun_SCSI_DEV_WWID=3600507640081818ab0000000000004d0
```

### Attribute description

This section describes the attributes that are available for iscsi-lun
resources. The value of these attributes can be evaluated in test programs
(see 'Environment variables' above) and for defining test requirements
(see 'Attribute conditions' in [Test resources](../resources.md)).

  - **`system/iscsi-lun/id:`** *(type: scalar)*

    The device ID of the iscsi-lun.

    For example
    'ip-10.210.9.1:3260-iscsi-iqn.1986-03.com.ibm:2145.v7k06.node1-lun-12'.

  - **`system/iscsi-lun/ipaddress:`** *(type: scalar)*

    IP address portion of the target portal address for the iscsi-lun
    resource object.

  - **`system/iscsi-lun/port:`** *(type: scalar)*

    TCP port portion of the target portal address for the iscsi-lun
    resource object.

  - **`system/iscsi-lun/target:`** *(type: scalar)*

    iSCSI target name for the iscsi-lun resource object.

  - **`system/iscsi-lun/iscsilun:`** *(type: scalar)*

    iSCSI LUN for the iscsi-lun resource object.

  - **`system/iscsi-lun/allow_write:`**  *(type: scalar)*

    This attribute indicates whether a test case is allowed to change the
    contents of the iSCSI-attached SCSI device. Valid values are:

      - 0: Test case must not write to the SCSI device (default)
      - 1: Test case is allowed to write to the SCSI device

    Example: Test requires a SCSI device that may be written to

    ```YAML
    system:
      iscsi-lun a:
        allow_write: 1
    ```

    Note: A non-default value for this attribute must be specified manually
    in the test environment file as it cannot be determined automatically.

  - **`system/iscsi-lun/scsi_dev/sysfs:`** *(type: scalar)*

    Sysfs-path of the SCSI device associated with the iscsi-lun.

  - **`system/iscsi-lun/scsi_dev/hctl:`** *(type: scalar)*

    The SCSI device name in HCTL format (host:channel:target:lun).

    Note that this name is neither persistent nor globally unique.

  - **`system/iscsi-lun/scsi_dev/host:`** *(type: scalar)*

    Host portion of the SCSI device name.

  - **`system/iscsi-lun/scsi_dev/channel:`** *(type: scalar)*

    Channel portion of the SCSI device name.

  - **`system/iscsi-lun/scsi_dev/target:`** *(type: scalar)*

    Target portion of the SCSI device name.

  - **`system/iscsi-lun/scsi_dev/lun:`** *(type: scalar)*

    LUN portion of the SCSI device name.

  - **`system/iscsi-lun/scsi_dev/wwid:`** *(type: scalar)*

    The World Wide Identifier (WWID) of the logical unit (LU) associated with
    the iscsi-lun. This ID is persistent and guaranteed to be unique for
    every SCSI device.

  - **`system/iscsi-lun/scsi_dev/vendor:`** *(type: scalar)*

    Device vendor.

  - **`system/iscsi-lun/scsi_dev/model:`** *(type: scalar)*

    Device model information.

  - **`system/iscsi-lun/scsi_dev/type:`** *(type: number)*

    SCSI Peripheral Device Type. Possible values for this attribute are defined
    in the SCSI Primary Commands standard.

    Example values:

      - 0: Direct access block device (e.g. disk)
      - 1: Sequential-access device (e.g. tape).
      - 8: Media changer device (e.g. tape-library robot).

    Example: Test requires an iSCSI-attached SCSI disk:

    ```YAML
    system:
      iscsi-lun a:
        scsi_dev:
          type: 0
    ```

  - **`system/iscsi-lun/scsi_dev/scsi_disk/protection_type:`** *(type: number)*

    SCSI disk T10 DIF protection type. Possible values for this attribute are
    defined in the SCSI Block Commands standard. This attribute is only
    available for SCSI disks.

    Example values:

      - 0: T10 DIF type 0: no protection.
      - 1: T10 DIF type 1: protection using logical block guard field
                           and logical block reference tag field.
      - 2: T10 DIF type 2.
      - 3: T10 DIF type 3.

    Example: Test requires an iSCSI-attached SCSI disk with T10 DIF type 2:

    ```YAML
    system:
      iscsi-lun a:
        scsi_dev:
          type: 0
          scsi_disk:
            protection_type: 2
    ```

  - **`system/iscsi-lun/scsi_dev/scsi_disk/thin_provisioning:`** *(type: number)*

    SCSI disk logical block provisioning management enabled (LBPME) as
    defined in the SCSI Block Commands standard. This attribute is only
    available for SCSI disks.

    Example values:

      - 0: fully provisioned.
      - 1: thin or resource provisioned.

    Example: Test requires a thin provisioned iSCSI-attached SCSI disk:

    ```YAML
    system:
      iscsi-lun a:
        scsi_dev:
          type: 0
          scsi_disk:
            thin_provisioning: 1
    ```

  - **`system/iscsi-lun/block_dev/bdev:`** *(type: scalar)*

    Block device name. This attribute is only available for SCSI disks.

  - **`system/iscsi-lun/block_dev/size:`** *(type: number)*

    Total block device size in bytes. This attribute is only available for
    SCSI disks.

  - **`system/iscsi-lun/block_dev/blksize:`** *(type: number)*

    Size of one block. This attribute is only available for SCSI disks.

  - **`system/iscsi_lun_count_available:`** *(type: number)*

    Number of iscsi-lun resource objects that are available for test cases.

  - **`system/iscsi_lun_count_total:`** *(type: number)*

    Total number of iscsi-lun resource objects that are defined on the
    test system.

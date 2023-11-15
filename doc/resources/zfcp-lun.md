zfcp-lun resource
=================

A tela **zfcp-lun** resource represents a zfcp-attached SCSI device. It
provides the following attributes:

```YAML
system:
  zfcp-lun:
    id: <devid>
    fcpdev: <fcpdev_busid>
    wwpn: <wwpn>
    fcplun: <fcp_lun>
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

Additionally the following system attributes related to **zfcp-lun** resources
are provided:
```YAML
system:
  zfcp_lun_count_available: <number>
  zfcp_lun_count_total: <number>
```

### Environment variables

Test programs can access actual values of zfcp-lun attributes using
environment variables named:

```
   TELA_SYSTEM_ZFCP_LUN_<name>_<path>
```

where

  - `<name>` is the symbolic name given to an zfcp-lun resource in the test
     YAML file
  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_ZFCP_LUN_COUNT_AVAILABLE=1
TELA_SYSTEM_ZFCP_LUN_COUNT_TOTAL=8
TELA_SYSTEM_ZFCP_LUN_mylun=0.0.194d:0x500507630718c5e3:0x4004409b00000000
TELA_SYSTEM_ZFCP_LUN_mylun_BLOCK_DEV_BDEV=sdc
TELA_SYSTEM_ZFCP_LUN_mylun_BLOCK_DEV_BLKSIZE=512
TELA_SYSTEM_ZFCP_LUN_mylun_BLOCK_DEV_SIZE=21474836480
TELA_SYSTEM_ZFCP_LUN_mylun_FCPDEV=0.0.194d
TELA_SYSTEM_ZFCP_LUN_mylun_FCPLUN=0x4004409b00000000
TELA_SYSTEM_ZFCP_LUN_mylun_ID=0.0.194d:0x500507630718c5e3:0x4004409b00000000
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_CHANNEL=0
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_HOST=1
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_LUN=1083916292
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_MODEL=2107900
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_SCSI_DISK_PROTECTION_TYPE=1
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_SCSI_DISK_THIN_PROVISIONING=1
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_HCTL=1:0:0:1083916292
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_SYSFS=/sys/bus/scsi/devices/1:0:0:1083916292/
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_TARGET=0
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_TYPE=0
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_VENDOR=IBM
TELA_SYSTEM_ZFCP_LUN_mylun_SCSI_DEV_WWID=naa.6005076307ffc5e3000000000000049b
TELA_SYSTEM_ZFCP_LUN_mylun_WWPN=0x500507630718c5e3
```

### Attribute description

This section describes the attributes that are available for zfcp-lun
resources. The value of these attributes can be evaluated in test programs
(see 'Environment variables' above) and for defining test requirements
(see 'Attribute conditions' in [Test resources](../resources.md)).

  - **`system/zfcp-lun/id:`** *(type: scalar)*

    The device ID of the zfcp-lun.

    For example '0.0.194d:0x500507630718c5e3:0x4004409b00000000'.

  - **`system/zfcp-lun/fcpdev:`** *(type: scalar)*

    The bus-ID of the FCP device associated with the zfcp-lun. For example
    '0.0.194d'.

  - **`system/zfcp-lun/wwpn:`** *(type: scalar)*

    World wide port name (WWPN) of the target port associated with the
    zfcp-lun.

    Example: Test case requires two zfcp-luns on the same FCP device but with
    different WWPNs.

    ```YAML
    system:
      zfcp-lun a:
        fcpdev: %{fcpdev}
        wwpn: %{wwpn}
      zfcp-lun b:
        fcpdev: %{fcpdev}
        wwpn: != %{wwpn}
    ```

  - **`system/zfcp-lun/fcplun:`** *(type: scalar)*

    FCP LUN of the zfcp-lun resource object.

  - **`system/zfcp-lun/allow_write:`**  *(type: scalar)*

    This attribute indicates whether a test case is allowed to change the
    contents of the zfcp-attached SCSI device. Valid values are:

      - 0: Test case must not write to the SCSI device (default)
      - 1: Test case is allowed to write to the SCSI device

    Example: Test requires a SCSI device that may be written to

    ```YAML
    system:
      zfcp-lun a:
        allow_write: 1
    ```

    Note: A non-default value for this attribute must be specified manually
    in the test environment file as it cannot be determined automatically.

  - **`system/zfcp-lun/scsi_dev/sysfs:`** *(type: scalar)*

    Sysfs-path of the SCSI device associated with the zfcp-lun.

  - **`system/zfcp-lun/scsi_dev/hctl:`** *(type: scalar)*

    The SCSI device name in HCTL format (host:channel:target:lun).

    Note that this name is neither persistent nor globally unique.

  - **`system/zfcp-lun/scsi_dev/host:`** *(type: scalar)*

    Host portion of the SCSI device name.

  - **`system/zfcp-lun/scsi_dev/channel:`** *(type: scalar)*

    Channel portion of the SCSI device name.

  - **`system/zfcp-lun/scsi_dev/target:`** *(type: scalar)*

    Target portion of the SCSI device name.

  - **`system/zfcp-lun/scsi_dev/lun:`** *(type: scalar)*

    LUN portion of the SCSI device name.

  - **`system/zfcp-lun/scsi_dev/wwid:`** *(type: scalar)*

    The World Wide Identifier (WWID) of the logical unit (LU) associated with
    the zfcp-lun. This ID is persistent and guaranteed to be unique for every
    SCSI device.

    Example: Testcase requires two zfcp-luns on different FCP devices that
    address the same logical unit:

    ```YAML
    system:
      zfcp-lun a:
        fcpdev: %{fcpdev}
        scsi_dev:
          wwid: %{wwid}
      zfcp-lun b:
        fcpdev: != %{fcpdev}
        scsi_dev:
          wwid: %{wwid}
    ```

  - **`system/zfcp-lun/scsi_dev/vendor:`** *(type: scalar)*

    Device vendor.

  - **`system/zfcp-lun/scsi_dev/model:`** *(type: scalar)*

    Device model information.

  - **`system/zfcp-lun/scsi_dev/type:`** *(type: number)*

    SCSI Peripheral Device Type. Possible values for this attribute are defined
    in the SCSI Primary Commands standard.

    Example values:

      - 0: Direct access block device (e.g. disk)
      - 1: Sequential-access device (e.g. tape).
      - 8: Media changer device (e.g. tape-library robot).

    Example: Test requires a zfcp-attached SCSI disk:

    ```YAML
    system:
      zfcp-lun a:
        scsi_dev:
          type: 0
    ```

  - **`system/zfcp-lun/scsi_dev/scsi_disk/protection_type:`** *(type: number)*

    SCSI disk T10 DIF protection type. Possible values for this attribute are
    defined in the SCSI Block Commands standard. Zfcp supports 0 or 1. This
    attribute is only available for SCSI disks.

    Example values:

      - 0: T10 DIF type 0: no protection.
      - 1: T10 DIF type 1: protection using logical block guard field
                           and logical block reference tag field.

    Example: Test requires a zfcp-attached SCSI disk with T10 DIF type 1:

    ```YAML
    system:
      zfcp-lun a:
        scsi_dev:
          type: 0
          scsi_disk:
            protection_type: 1
    ```

  - **`system/zfcp-lun/scsi_dev/scsi_disk/thin_provisioning:`** *(type: number)*

    SCSI disk logical block provisioning management enabled (LBPME) as
    defined in the SCSI Block Commands standard. This attribute is only
    available for SCSI disks.

    Example values:

      - 0: fully provisioned.
      - 1: thin or resource provisioned.

    Example: Test requires a thin provisioned zfcp-attached SCSI disk:

    ```YAML
    system:
      zfcp-lun a:
        scsi_dev:
          type: 0
          scsi_disk:
            thin_provisioning: 1
    ```

  - **`system/zfcp-lun/block_dev/bdev:`** *(type: scalar)*

    Block device name. This attribute is only available for SCSI disks.

  - **`system/zfcp-lun/block_dev/size:`** *(type: number)*

    Total block device size in bytes. This attribute is only available for
    SCSI disks.

  - **`system/zfcp-lun/block_dev/blksize:`** *(type: number)*

    Size of one block. This attribute is only available for SCSI disks.

  - **`system/zfcp_lun_count_available:`** *(type: number)*

    Number of zfcp-lun resource objects that are available for test cases.

  - **`system/zfcp_lun_count_total:`** *(type: number)*

    Total number of zfcp-lun resource objects that are defined on the
    test system.

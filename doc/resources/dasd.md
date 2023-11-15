dasd resource
=============

A **dasd** resource represents a FICON DASD volume. It provides the following
attributes:

```YAML
system:
  dasd:
    busid: <busid>
    type: eckd|fba
    sysfs: <sysfs_path>
    online: <online_state>
    cutype: <cutype_and_model>
    devtype: <devtype_and_model>
    uid:
      id: <full_uid>
      vendor: <uid_vendor>
      serial: <uid_serial>
      ssid: <uid_ssid>
      ua: <uid_unit_address>
      vduit: <vm_uid_portion>
    chpid:
      <chpid_data>
    chpid_count: <available_chpids>
    alias_count: <available_aliases>
    block_dev:
      bdev: <blockdev_name>
      size: <blockdev_size>
      blksize: <blocksize>
    format: cdl|ldl|none
    cylinder_count: <number_of_cylinders>
    fc_security: unsupported|authentication|encryption|inconsistent|-
    allow_write: <flag>
```
Additionally the following system attributes related to **dasd** resources
are provided:
```YAML
system:
  dasd_count_available: <number>
  dasd_count_total: <number>
```

Note that the **dasd** resource does not include HyperPAV and PAV DASD alias
devices. The availability of such devices for a DASD volume can be determined
using the `alias_count` attribute.

### Environment variables

Test programs can access actual values of dasd attributes using environment
variables named:

```
   TELA_SYSTEM_DASD_<name>_<path>
```

where

  - `<name>` is the symbolic name given to a dasd in the test YAML file
  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_DASD_mydasd=0.0.e880
TELA_SYSTEM_DASD_mydasd_ALLOW_WRITE=0
TELA_SYSTEM_DASD_mydasd_BLOCK_DEV_BDEV=dasdb
TELA_SYSTEM_DASD_mydasd_BLOCK_DEV_BLKSIZE=4096
TELA_SYSTEM_DASD_mydasd_BLOCK_DEV_SIZE=22156001280
TELA_SYSTEM_DASD_mydasd_BUSID=0.0.e880
TELA_SYSTEM_DASD_mydasd_CUTYPE=3990/e9
TELA_SYSTEM_DASD_mydasd_DEVTYPE=3390/0c
TELA_SYSTEM_DASD_mydasd_FORMAT=cdl
TELA_SYSTEM_DASD_mydasd_ALIAS_COUNT=16
TELA_SYSTEM_DASD_mydasd_CYLINDER_COUNT=30051
TELA_SYSTEM_DASD_mydasd_ONLINE=1
TELA_SYSTEM_DASD_mydasd_SYSFS=/sys/bus/ccw/drivers/dasd-eckd/0.0.e880
TELA_SYSTEM_DASD_mydasd_TYPE=eckd
TELA_SYSTEM_DASD_mydasd_UID_ID=IBM.750000000DL241.e800.80
TELA_SYSTEM_DASD_mydasd_UID_SERIAL=750000000DL241
TELA_SYSTEM_DASD_mydasd_UID_SSID=e800
TELA_SYSTEM_DASD_mydasd_UID_UA=80
TELA_SYSTEM_DASD_mydasd_UID_VENDOR=IBM
```

### Attribute description

This section describes the attributes that are available for dasd resources.
The value of these attributes can be used in test programs (see 'Environment
variables' above) and for defining test requirements (see 'Attribute
conditions' in [Test resources](../resources.md)).

  - **`system/dasd/busid:`**  *(type: scalar)*

    The bus-ID of the DASD volume.

  - **`system/dasd/type:`**  *(type: scalar)*

    DASD volume type. Valid values are 'eckd' and 'fba'.

  - **`system/dasd/sysfs:`**  *(type: scalar)*

    Sysfs-path associated with the DASD volume.

  - **`system/dasd/online:`**  *(type: scalar)*

    Online state of the DASD volume. Valid values are 0 for 'offline' and 1 for
    'online'. Note that if a test YAML file does not contain a condition for
    the online attribute, both online and offline DASD volumes will match.

    Example: Test requires an online DASD volume

    ```YAML
    system:
      dasd mydasd:
        online: 1
    ```

  - **`system/dasd/cutype:`**  *(type: scalar)*

    Control unit type and model information for the DASD volume, for example
    '3990/e9'.

  - **`system/dasd/devtype:`**  *(type: scalar)*

    Device type and model information for the DASD volume, for example
    '3390/0c'.

  - **`system/dasd/uid/id:`**  *(type: scalar)*

    The full DASD volume UID.

    Example: IBM.750000000XD621.2e00.4f.00001d4c00001daf0000000000000000

    Note: There are separate attributes available for each component of the
    DASD UID.

  - **`system/dasd/uid/vendor:`**  *(type: scalar)*

    Vendor component of the DASD volume UID.

  - **`system/dasd/uid/serial:`**  *(type: scalar)*

    Serial number component of the DASD volume UID.

  - **`system/dasd/uid/ssid:`**  *(type: scalar)*

    SSID component of the DASD volume UID.

    Example: Test requires two DASDs on the same subsystem.

    ```YAML
    system:
      dasd a:
        uid:
          vendor: %{vendor}
          serial: %{serial}
          ssid: %{ssid}
      dasd b:
        uid:
          vendor: %{vendor}
          serial: %{serial}
          ssid: %{ssid}
    ```

  - **`system/dasd/uid/ua:`**  *(type: scalar)*

    Unit address component of the DASD volume UID.

  - **`system/dasd/uid/vduit:`**  *(type: scalar)*

    Virtual disk unique-identifier token component of the DASD volume UID.
    This attribute is only available when the DASD volume is a z/VM minidisk.

  - **`system/dasd/chpid:`** *(type: object)*

    A CHPID that is defined for the DASD volume. See [chpid resource](chpid.md)
    for a list of available chpid resource attributes.

    Example: Test requires a DASD volume with at least one CHPID that can be
    varied offline.

    ```YAML
    system:
      dasd mydasd:
        chpid mychpid:
          allow_offline: 1
    ```

    Note: By listing a DASD volume in the resource YAML file, all associated
    CHPIDs are automatically registered as available for testing. Also note that
    to override attribute values of a DASD's CHPID in a resource YAML file,
    the attributes must be specified in the corresponding CHPID section
    (system/chpid) and not in the DASD's CHPID section (system/dasd/chpid).


  - **`system/dasd/chpid_count:`** *(type: number)*

    Number of CHPIDs defined for the DASD volume.

    Example: Test requires a DASD volume with at least two CHPIDs.

    ```YAML
    system:
      dasd a:
        chpid_count: >=2
    ```

  - **`system/dasd/alias_count:`**  *(type: number)*

    The number of PAV and HyperPAV aliases that are available for I/O to the
    DASD volume.

  - **`system/dasd/block_dev/bdev:`**  *(type: scalar)*

    Block device name. This attribute is only available when the DASD volume is
    online.

  - **`system/dasd/block_dev/blksize:`**  *(type: number)*

    Size of one block in bytes. This attribute is only available when the DASD
    volume is online and formatted.

  - **`system/dasd/block_dev/size:`**  *(type: number)*

    Total formatted block device size in bytes. This attribute is only
    available when the DASD is online and formatted.

    Note that when specifying a condition for this attribute in a test YAML
    file you can use suffixes Ki, Mi, Gi and Ti to indicate sizes in units of
    KiB, MiB, GiB and TiB.

    Example: Test requires a 1 TiB DASD volume
    ```YAML
    system:
      dasd large_volume:
        block_dev:
          size: >= 1TiB
    ```

  - **`system/dasd/format:`**  *(type: scalar)*

    DASD format type. Valid values are
      - cdl: Compatible Disk Layout
      - ldl: Linux Disk Layout
      - none: unformatted

    This attribute is only available when the DASD is online.

    Example: Test requires an unformatted ECKD DASD.

    ```YAML
    system:
      dasd mydasd:
        type: eckd
        format: none
    ```

  - **`system/dasd/cylinder_count:`**  *(type: number)*

    Unformatted DASD volume capacity in cylinders. This attribute is only
    available when the DASD is online.

  - **`system/dasd/allow_write:`**  *(type: scalar)*

    This attribute indicates whether a test case is allowed to change the
    contents of the DASD volume. Valid values are:

      - 0: Test case must not write to DASD volume (default)
      - 1: Test case is allowed to write to DASD volume

    Note that even with a value of 1, the format of a DASD after the test case
    ends must be the same as at the start of the test case.

    Example: Test requires a DASD volume that may be written to

    ```YAML
    system:
      dasd a:
        allow_write: 1
    ```

    Note: A non-default value for this attribute must be specified manually
    in the test environment file as it cannot be determined automatically.

  - **`system/dasd_count_available:`** *(type: number)*

    Number of dasd resource objects that are available for test cases.

  - **`system/dasd_count_total:`** *(type: number)*

    Total number of dasd resource objects that are defined on the
    test system.

    Example: Test requires a system with no DASD volume.

    ```YAML
    system:
      dasd_count_total: 0
    ```

  - **`system/dasd/fc_security:`** *(type: scalar)*

    This attribute indicates the state of the Fibre Channel Endpoint Security
    feature for a DASD volume. Valid values are:

      - unsupported: The DASD device does not support FCES
      - authentication: The connection has been authenticated
      - encryption: The connection is encrypted
      - inconsistent: The operational channel paths of the DASD device report
                      inconsistent FCES status
      - -: Flag is not available

    Example: Test requires a system with a DASD volume where the fc_security
    attribute is available.

    ```YAML
    system:
      dasd a:
        fc_security: != -
    ```

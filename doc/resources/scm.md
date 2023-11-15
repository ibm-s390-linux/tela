scm resource
============

An **scm** resource represents a Storage Class Memory increment. It provides
the following attributes:

```YAML
system:
  scm:
    busid: <busid>
    sysfs: <sysfs_path>
    opstate: <oper_state>
    block_dev:
      bdev: <blockdev_name>
      size: <blockdev_size>
    allow_write: <flag>
```
Additionally the following system attributes related to the **scm** resources
are provided:
```YAML
system:
  scm_count_available: <number>
  scm_count_total: <number>
```

### Environment variables

Test programs can access actual values of the scm attributes using the
environment variables named:

```
	TELA_SYSTEM_SCM_<name>_<path>
```

Where

  - `<name>` is the symbolic name given to the scm increment in the test YAML
     file
  - `<path>` is the attribute's uppercase YAML path with underscores in place
     of non-alphanumeric characters.

Example:
```
TELA_SYSTEM_SCM_myscm=0000000000000000
TELA_SYSTEM_SCM_myscm_BUSID=0000000000000000
TELA_SYSTEM_SCM_myscm_SYSFS=/sys/bus/scm/drivers/scm_block/0000000000000000
TELA_SYSTEM_SCM_myscm_OPSTATE=1
TELA_SYSTEM_SCM_myscm_BLOCK_DEV_BDEV=scma
TELA_SYSTEM_SCM_myscm_BLOCK_DEV_SIZE=17179869184
TELA_SYSTEM_SCM_myscm_BLOCK_DEV_BLKSIZE=4096
```

### Attribute description

This section describes the attributes that are available for scm resources.
The values of these attributes can be used in test programs (see 'Environment
variables' above) and for defining the test requirements (see 'Attribute
conditions' in [Test resources](../resources.md)).

  - **`system/scm/busid:`** *(type: scalar)*

    The bus-ID of the SCM increment.

  - **`system/scm/sysfs:`** *(type: scalar)*

    Sysfs-path associated with the SCM increment.

  - **`system/scm/opstate:`**  *(type: number)*

    Operational state of the SCM increment.

  - **`system/scm/block_dev/bdev:`**  *(type: scalar)*

    Block device name.

  - **`system/scm/block_dev/size:`**  *(type: number)*

    Total block device size in bytes.

    Note that when specifying a condition for this attribute in a test YAML
    file you can use suffixes Ki, Mi, Gi and Ti to indicate sizes in units of
    KiB, MiB, GiB and TiB.

    Example: Test requires a 1 MiB SCM increment
    ```YAML
    system:
      scm *:
        block_dev:
          size: >= 1MiB
    ```

  - **`system/scm/allow_write:`**  *(type: scalar)*

    This attribute indicates whether a test case is allowed to change the
    contents of the SCM increment. Valid values are:

      - 0: Test case must not write to SCM increment (default)
      - 1: Test case is allowed to write to SCM increment

    Example: Test requires an SCM increment that may be written to

    ```YAML
    system:
      scm myscm:
        allow_write: 1
    ```

    Note: A non-default value for this attribute must be specified manually
    in the test environment file as it cannot be determined automatically.

  - **`system/scm_count_available:`** *(type: number)*

    Number of scm resource objects that are available for test cases.

  - **`system/scm_count_total:`** *(type: number)*

    Total number of scm resource objects that are defined on the
    test system.

    Example: Test requires a system with no SCM increment.

    ```YAML
    system:
      scm_count_total: 0
    ```

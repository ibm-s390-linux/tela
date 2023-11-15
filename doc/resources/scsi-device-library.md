SCSI device library
===================

The tela framework provides helper functions in a library.
The functions described here are for tela resources with an associated
SCSI device. An example resource is **zfcp-lun**.

### Bash programs

For Bash programs, function implementations are available by sourcing the
$TELA\_LIB/scsi-device file:

```Bash
source $TELA_LIB/scsi-device || exit 1
```

### Available Functions:

  - `enrich_env_for_sdev(env_var_prefix)`

    @env_var_prefix: Prefix string of matched tela resource, e.g.
                     TELA\_SYSTEM\_<resource-type>\_<match-name>

    Sets additional environment variables with the same variable name prefix as
    @env_var_prefix but different suffixes. The additional variables provide
    SCSI device or SCSI high-level driver properties of the associated resource.
    The variables are only set if the value source exists in the system.
    These properties are not part of the tela resource as they are not
    applicable for resource matching. Variables by suffix:

    - **`_SCSI_DEV_STATE`**

      State of SCSI device. This should always exist.

    - **`_SG_DEV_CDEV`**

      Name of character device for corresponding SCSI generic device.

    - **`_ST_DEV_CDEV`**

      Name of character device for corresponding SCSI tape drive
      driven by the Linux SCSI tape device driver.

    - **`_CH_DEV_CDEV`**

      Name of character device for corresponding SCSI medium changer device
      driven by the Linux SCSI medium changer device driver.

    - **`_LIN_TAPE_DEV_CDEV`**

      Name of character device for corresponding tape drive or medium changer
      driven by the IBM lin_tape device driver.

    - **`_LIN_TAPE_DEV_SERIAL`**

      Serial number string for corresponding tape drive or medium changer
      driven by the IBM lin_tape device driver.

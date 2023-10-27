dcss resource
=============

A tela **dcss** resource represents a Discontiguous Saved Segment (DCSS).
It provides the following attributes:

```YAML
system:
  dcss:
    name: <segment_name>
    begin: <start_address>
    end: <end_address>
    size: <size_bytes>
    type: <segment_type>
```
Additionally the following system attributes related to **dcss** resources
are provided:
```YAML
system:
  dcss_count_available: <number>
  dcss_count_total: <number>
```

### Environment variables

Test programs can access actual values of dcss attributes using environment
variables named:

```
   TELA_SYSTEM_DCSS_<name>_<path>
```

where

  - `<name>` is the symbolic name given to an dcss resource in the test YAML
    file
  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_DCSS_mydcss=LNXCI1
TELA_SYSTEM_DCSS_mydcss_NAME=LNXCI1
TELA_SYSTEM_DCSS_mydcss_BEGIN=0x400100000
TELA_SYSTEM_DCSS_mydcss_END=0x400200000
TELA_SYSTEM_DCSS_mydcss_SIZE=1048576
TELA_SYSTEM_DCSS_mydcss_TYPE=SW
```

### Attribute description

This section describes the attributes that are available for dcss resources.
The value of these attributes can be evaluated in test programs
(see 'Environment variables' above) and for defining test requirements
(see 'Attribute conditions' in [Test resources](../resources.md)).


  - **`system/dcss/name:`**  *(type: scalar)*

    The name of the DCSS. For example 'LNXCI1'.

  - **`system/dcss/begin:`**  *(type: scalar)*

    The start memory address of the DCSS. For example '0x400100000'.

  - **`system/dcss/end:`**  *(type: scalar)*

    The end memory address of the DCSS. For example '0x400200000'.

  - **`system/dcss/size:`**  *(type: scalar)*

    The size of the DCSS in bytes. For example '1048576'.

    Note that when specifying a condition for this attribute in a test YAML
    file you can use suffixes Ki, Mi, Gi and Ti to indicate sizes in units of
    KiB, MiB, GiB and TiB.

  - **`system/dcss/type:`**  *(type: scalar)*

    The type of the DCSS. For example 'SW'.
    More details can be found in z/VM `CP Commands and Utilities Reference`.

  - **`system/dcss_count_available:`** *(type: number)*

    Number of DCSS resource objects that are available for test cases.

  - **`system/dcss_count_total:`** *(type: number)*

    Total number of DCSS resource objects that are defined on the
    test system.

    Example: Test requires a system with no DCSS segment.

    ```YAML
    system:
      dcss_count_total: 0
    ```

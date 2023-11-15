apqn resource
=============

A tela **apqn** resource represents an Adjunct Processor Queue Number
of a Crypto Express adapter. It provides the following attributes:

```YAML
system:
  apqn:
    busid: <busid>
    sysfs: <sysfs_path>
    card: <card_busid>
    domain: <domain_id>
    mode: cca|accel|ep11
    type: <card_type>
    cex_level: <cex_level>
    online: <online_state>
    raw_hwtype: <raw_hw_type_id>
    ap_functions: <functions_bitfield>
```

### Environment variables

Test programs can access actual values of apqn attributes using environment
variables named:

```
   TELA_SYSTEM_APQN_<name>_<path>
```

where

  - `<name>` is the symbolic name given to an apqn resource in the test YAML
    file
  - `<path>` is an attribute's uppercase YAML path with underscores in place of
    non-alphanumeric characters.

Example:

```
TELA_SYSTEM_APQN_myapqn=03.003d
TELA_SYSTEM_APQN_myapqn_BUSID=03.003d
TELA_SYSTEM_APQN_myapqn_SYSFS=/sys/bus/ap/devices/03.003d
TELA_SYSTEM_APQN_myapqn_CARD=03
TELA_SYSTEM_APQN_myapqn_DOMAIN=003d
TELA_SYSTEM_APQN_myapqn_MODE=cca
TELA_SYSTEM_APQN_myapqn_TYPE=CEX5C
TELA_SYSTEM_APQN_myapqn_CEX_LEVEL=5
TELA_SYSTEM_APQN_myapqn_ONLINE=1
TELA_SYSTEM_APQN_myapqn_RAW_HWTYPE=11
TELA_SYSTEM_APQN_myapqn_AP_FUNCTIONS=0x92000000
```

### Attribute description

This section describes the attributes that are available for apqn resources.
The value of these attributes can be evaluated in test programs
(see 'Environment variables' above) and for defining test requirements
(see 'Attribute conditions' in [Test resources](../resources.md)).


  - **`system/apqn/busid:`**  *(type: scalar)*

    The bus-ID of the APQN. For example '03.003d'.

  - **`system/apqn/sysfs:`**  *(type: scalar)*

    Sysfs-path associated with the APQN.

  - **`system/apqn/card:`**  *(type: scalar)*

    The bus-ID of the card associated with the APQN.

    Example: Test requires two APQNs on different cards

    ```YAML
    system:
      apqn a:
        card: %{id}
      apqn b:
        card: != %{id}
    ```

  - **`system/apqn/domain:`**  *(type: scalar)*

    The bus-ID of the domain associated with the APQN.

    Example: Test requires two APQNs with the same domain bus-ID

    ```YAML
    system:
      apqn a:
        domain: %{id}
      apqn b:
        domain: %{id}
    ```

  - **`system/apqn/mode:`**  *(type: scalar)*

    Textual card type, for example 'CEX6A'.

  - **`system/apqn/cex_level:`**  *(type: number)*

    The numerical Crypto Express card level. This number can be used to specify
    a requirement for a minimum card level in a test YAML file.

    Example: Test requires CEX3 or later

    ```YAML
    system:
      apqn my_apqn:
        cex_level: >=3
    ```

  - **`system/apqn/online:`**  *(type: scalar)*

    Online state of the APQN. Valid values are 0 for 'offline' and 1 for
    'online'. Note that if a test YAML file does not contain a condition for
    the online attribute, both online and offline APQNs will match.

    Example: Test requires an online APQN

    ```YAML
    system:
      apqn myapqn:
        online: 1
    ```

  - **`system/apqn/raw_hwtype:`**  *(type: number)*

    The numerical representation of the cryptographic adapter's actual hardware
    type.

  - **`system/apqn/ap_functions:`**  *(type: number)*

    A number representing the function facilities that are installled on the
    adapter.

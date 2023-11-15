Environment variables
=====================

The tela framework provides the following environment variables for use by test
programs:

  - TELA\_BASH

    Bash scripts can use this variable to locate the file providing Bash
    versions of the helper functions described in
    [Helper Functions](functions.md).

    Example usage:

    ```
    source $TELA_BASH || exit 1
    ```

  - TELA\_SCOPE

    Specifies the scope of tests to be performed.  The value designates
    the typical runtime of a single test.  Valid values are:

    - 'quick' for runtimes less than 1 minute
    - 'full' for runtimes longer than 1 minute

    The default value is 'quick'.

  - TELA\_EXEC

    Absolute path to the test program.

  - TELA\_OS\_ID
    TELA\_OS\_VERSION

    ID and version of the operating system used on the local system. These
    fields correspond to the ID and VERSION\_ID fields as defined in the
    os-release file (see 'man os-release').

    Note: You can manually run the 'src/libexec/os' tool to get these IDs for a
    specific environment.

  - TELA\_TMP

    The path to a temporary directory for use by test programs. The directory
    is initially empty. It will be automatically deleted after the test program
    exits.

    Note: Set the `large_temp` YAML key if you intend to store more than
    about 100 MB of temporary data in this directory. See [YAML file](yaml.md)
    for more information on YAML keys.

  - TELA\_RESOURCE\_FILE

    The name of a YAML file that lists details about the resources available
    for use by the test program.

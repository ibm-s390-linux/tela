YAML file
=========

Each test executable can be accompanied by a YAML file that provides additional
information about the implemented testcases and lists test resource
requirements. The format of this file is a subset of [YAML][1] (basically YAML
with only block collection styles). The YAML file must be named like the test
executable name with a '.yaml' suffix.

The basic structure of this tela YAML file looks like this:

```YAML
section1:
  key1: <value>
  key2: <value>
section2:
  key1: <value>
  key2: <value>
...
```

[1]: http://yaml.org

### YAML path

A special notation is used throughout the tela documentation to specify the
structural location of a YAML element inside a YAML document. This notation is
similar to a file path, where each path component represents a parent YAML
mapping.

Example:

The following YAML Path

```
  section/subsection/key
```

identifies a YAML element that is found at:

```YAML
section:
  subsection:
    key:
```

### Section 'test'

This section contains information related to the test executable. Supported
keys are:

  - **`test/plan:`** *(type: number)*

     The number of testcases implemented by this test executable.

     Example for a YAML file announcing 9 testcases:

        test:
          plan: 9

  - **`test/plan/<name>:`** *(type: string)*

     The description of the tests. Will be added to test result.
     The Entries will be matched to either the testname or the name of the
     executable, if the test doesn't generates TAP13-Output.
     This replaces the number in 'test/plan'

     Example for TAP13 Testexecs:

        test:
          plan:
            tela_tmp: "Check if TELA_TMP is set"
            tela_tmp_empty: "Check if the tmp directory is empty"
            tela_tmp_writable: "Check if the tmp directory is writable"

     Example for Non-TAP13 Testexecs:

        test:
          plan:
            simple.sh: "A simple test"

  - **`test/large_temp:`** *(type: number)*

     If set to 1, the temporary directory specified by environment variable
     `TELA_TMP` is placed in a file system location better suited for large
     amounts of data. Otherwise the default location for temporary directories
     is used, which may be size-restricted.

     Test authors should set this key to 1 if they intend to store more than
     about 100 MB of temporary data.


### Resource sections

See [test resources](resources.md) for more information on YAML sections that
describe how to define test resource requirements in the test YAML file.

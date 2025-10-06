# Automated tests

## test_modules.sh
Runs the module tests. Each test case has a subdirectory in module\_tests. The test cases are run as follows:
1. run every shell script (\*.sh) in the subdirectory
2. check the pipeline log for errors
3. for every reference file (\<basename\>\_out-ref.dat), find the output file (\<basename\>\_ref.dat) and compare them

### Failures due to minimal differences in output
Some tests may fail due to differences in floating point optimizations between CPU architectures.
To avoid this, use a debug build for running tests (and activate sanitizers):
```sh
cd SRC
make clean
make install DEBUG=2
```

## Developing module tests
To keep everything clean and running smoothly:
- add the files that are needed to run the tests
- add the files that are needed to create the input data
- do not add output files to version control
- output file names should either end with \_out.dat, or should be added to .gitignore

## test_examples.sh
Executes the pipeline of the instruments in the FILES/EXAMPLES directory. The pipelines commands are in tests/examples/run_<name>.sh, where <name> corresponds to a subdirectory of FILES/EXAMPLES. After the pipeline, the script tests/examples/check_<name>.sh is run to perform tests on the results. When adding examples, make sure that visualization is off, that the pipeline runs in a reasonable time and that the exported shell script is executable.

## test_yaml_format.py
Check if the YAML files are well-formed and if the referenced executables exist. TODO: this should also check for semantic errors.

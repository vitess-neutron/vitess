# Automated tests

## check_examples.sh
Executes the pipeline of the instruments in the FILES/EXAMPLES directory. The pipelines commands are in tests/examples/run_<name>.sh, where <name> corresponds to a subdirectory of FILES/EXAMPLES. After the pipeline, the script tests/examples/check_<name>.sh is run to perform tests on the results. When adding examples, make sure that visualization is off, that the pipeline runs in a reasonable time and that the exported shell script is executable.

## check_yaml.py
Check if the YAML files are well-formed and if the referenced executables exist. TODO: this should also check for semantic errors.

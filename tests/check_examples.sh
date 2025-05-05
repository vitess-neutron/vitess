#!/bin/sh

TEST_DIR="$(dirname "$0")"
VITESS_DIR="$(realpath "${TEST_DIR}/..")"
V="${VITESS_DIR}/MODULES"

cd "${TEST_DIR}/examples" || exit 1
FAIL=0
for PIPELINE in run*.sh; do
    # workaround until souce_vae tests run on darwin
    if [ "${PIPELINE}" = "run_SourceAI.sh" -a ! -x "${V}/source_vae_`uname -s`_`uname -m`" ]; then
        echo "Skipping ${PIPELINE} (module not found)"
        echo
        continue
    fi
    if [ -x "${PIPELINE}" ]; then
        NAME="$(basename "${PIPELINE#run_}" .sh)"
        CHECK="check_${NAME}.sh"
        P="${VITESS_DIR}/FILES/EXAMPLES/${NAME}"
        if [ -d "${P}" ]; then
            echo "Running pipeline for example ${NAME}..."
            V="${V}" P="${P}" L="/tmp/vitess_test_${NAME}" "./${PIPELINE}" || FAIL=1
            grep -m1 -B1 '^ERROR:' "${P}/result.txt" && FAIL=1
            grep -m1 'neutron count rate *: *0.0000e+00 +/- *0.000e+00 n/s' "${P}/result.txt" && FAIL=1
            if [ -x "${CHECK}" ]; then
                echo "Running checks for example ${NAME}..."
                V="${V}" P="${P}" L="/tmp/vitess_test_${NAME}" "./${CHECK}" || FAIL=1
            fi
        else
            echo "Error in test ${PIPELINE}: directory ${P} does not exist"
            FAIL=1
        fi
        echo
    else
        echo "Error: ${PIPELINE} is not executable"
        FAIL=1
    fi
done
exit ${FAIL}

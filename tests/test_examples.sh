#!/bin/sh

TEST_DIR="$(dirname "$0")"
VITESS_DIR="$(realpath "${TEST_DIR}/..")"
V="${VITESS_DIR}/MODULES"

cd "${TEST_DIR}/examples" || exit 1
RESULT=0
for PIPELINE in run*.sh; do
    FAIL=0
    # workaround until souce_ai tests run on darwin
    if [ "${PIPELINE}" = "run_SourceAI.sh" ] && [ ! -x "${V}/source_ai_$(uname -s)_$(uname -m)" ]; then
        echo "Skipping ${PIPELINE} (module not found)"
        continue
    fi
    if [ -x "${PIPELINE}" ]; then
        NAME="$(basename "${PIPELINE#run_}" .sh)"
        CHECK="check_${NAME}.sh"
        P="${VITESS_DIR}/FILES/EXAMPLES/${NAME}"
        if [ -d "${P}" ]; then
            echo "Running pipeline for example ${NAME}..."
            V="${V}" P="${P}" L="/tmp/vitess_test_${NAME}" "./${PIPELINE}"
            if [ $? -ne 0 ]; then
                echo "FAIL ${PIPELINE}: nonzero exit code"
                FAIL=1
            fi
            G="$(grep -m1 -B1 '^ERROR:' "${P}/result.txt")"
            if [ $? -eq 0 ]; then
                echo "FAIL ${PIPELINE}: error in result.txt"
                echo "$G"
                FAIL=1
            else
                G="$(grep -m1 'neutron count rate *: *0.0000e+00 +/- *0.000e+00 n/s' "${P}/result.txt")"
                if [ $? -eq 0 ]; then
                    echo "FAIL ${PIPELINE}: zero flux in result.txt"
                    echo "$G"
                    FAIL=1
                fi
            fi
            if [ ${FAIL} -eq 0 ]; then
                echo "OK   ${PIPELINE}"
            fi
            if [ -x "${CHECK}" ]; then
                V="${V}" P="${P}" L="/tmp/vitess_test_${NAME}" "./${CHECK}"
                if [ $? -eq 0 ]; then
                    echo "OK   ${CHECK}"
                else
                    echo "FAIL ${CHECK}"
                    FAIL=1
                fi
            fi
        else
            echo "FAIL directory ${P} does not exist"
            FAIL=1
        fi
    else
        echo "FAIL ${PIPELINE} is not executable"
        FAIL=1
    fi
    if [ ${FAIL} -ne 0 ]; then
        RESULT=1
    fi
done
exit ${RESULT}

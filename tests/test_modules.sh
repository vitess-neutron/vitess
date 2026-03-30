#!/bin/sh

. "$(dirname "$0")/common.sh"

main() {
    if [ $# -eq 0 ]; then
        test_directories "${TESTS_DIR}"/module_tests/*
    else
        test_directories "$@"
    fi
}

test_directories() {
    FAIL=0
    for DIR in "$@"; do
        [ -d "${DIR}" ] || continue
        [ "${DIR}" = "." ] && DIR="$(pwd)"
        check_dir "$(basename "${DIR}")" || FAIL=1
        run_pipelines "$(basename "${DIR}")" || FAIL=1
        if [ -z "${NO_DIFF}" ]; then
            check_output "$(basename "${DIR}")" || FAIL=1
        fi
    done
    return ${FAIL}
}

run_pipelines() {
    TEST_NAME="$1"
    P="${TESTS_DIR}/module_tests/${TEST_NAME}"
    PIPELINE_FAIL=0
    for PIPELINE in "${P}"/*.sh; do
        if [ ! -x "${PIPELINE}" ]; then
            echo "$(basename "${PIPELINE}") is not executable"
            continue
        fi
        [ "${PIPELINE%-win.sh}" = "${PIPELINE}" ] || continue
        SCRIPT="$(basename "${PIPELINE}")"
        [ -n "${FIX_SCRIPTS}" ] && _fix_scripts
        if check_script "${PIPELINE}"; then
            echo "Running pipeline ${SCRIPT} for module test ${TEST_NAME}..."
            # V="${V}" P="${P}" L="${TEMP_DIR}/vitess_test_${TEST_NAME}" "${PIPELINE}" || PIPELINE_FAIL=1
            V="${V}" P="${P}" L="${TEMP_DIR}/vitess_test_${TEST_NAME}" call_pipeline "${PIPELINE}" || PIPELINE_FAIL=1
            grep -m1 -B1 '^ERROR:' "${P}/result.txt" && PIPELINE_FAIL=1
            grep -m1 'neutron count rate *: *0.0000e+00 +/- *0.000e+00 n/s' "${P}/result.txt" && PIPELINE_FAIL=1
        else
            PIPELINE_FAIL=1
        fi
    done
    return ${PIPELINE_FAIL}
}

check_output() {
    TEST_NAME="$1"
    P="${TESTS_DIR}/module_tests/${TEST_NAME}"
    OUTPUT_FAIL=0
    for REF_FILE in "${P}"/*_out-ref.dat; do
        [ -r "${REF_FILE}" ] || continue
        OUT_NAME="$(basename "${REF_FILE}" | sed 's/_out-ref.dat/_out.dat/g')"
        OUT_FILE="${P}/${OUT_NAME}"
        # TODO: ignore certain parts of monitor files
        DIFF="$(diff -u0 -W140 --suppress-common-lines -b -I '^#' "${REF_FILE}" "${OUT_FILE}" 2>&1)"
        if [ $? -eq 0 ]; then
            echo "OK   ${TEST_NAME}/${OUT_NAME}"
        else
            echo "FAIL ${TEST_NAME}/${OUT_NAME}"
            echo "${DIFF}" | tail -n+3 | head -n6
            OUTPUT_FAIL=1
        fi
    done
    return ${OUTPUT_FAIL}
}

check_script() {
    SCRIPT_FAIL=0
    if [ ! -x "${PIPELINE}" ]; then
        echo "Script ${1} is not executable"
        SCRIPT_FAIL=1
    fi
    if grep -q '_out-ref.dat' "${1}"; then
        echo "Script ${1} contains reference file"
        SCRIPT_FAIL=1
    fi
    return ${SCRIPT_FAIL}
}

check_dir() {
    TEST_NAME="$1"
    P="${TESTS_DIR}/module_tests/${TEST_NAME}"
    DIR_FAIL=0
    if ! ls "${P}"/*.sh > /dev/null 2>&1; then
        echo "ERROR in ${TEST_NAME}: No pipeline script."
        DIR_FAIL=1
    fi
    if ! ls "${P}"/*_out-ref.dat > /dev/null 2>&1; then
        echo "ERROR in ${TEST_NAME}: No reference files."
        DIR_FAIL=1
    fi
    return ${DIR_FAIL}
}

if [ "${WINDOWS}" = "true" ]; then
    call_pipeline() {
        SCRIPT='/^\$\{?V\}?/ {
            s/(\$\{?P\}?[^[:space:]]*)/$(cygpath -w \1)/g;
            s/(\$\{?L\}?[^[:space:]]*)/$(cygpath -w \1)/g;
        }'
        WINPIPE="${1%.sh}-win.sh"
        sed -E "${SCRIPT}" "${PIPELINE}" > "${WINPIPE}"
        "${WINPIPE}"
    }
else
    call_pipeline() {
        "$1"
    }
fi

# developer only: fix pipeline scripts
_fix_scripts() {
    [ -x "${PIPELINE}" ] || chmod +x "${PIPELINE}"
    sed -i -e 's/_out-ref\.dat/_out.dat/g' \
           -e 's#V=.*/MODULES#V=/tmp/vitess/MODULES#' \
           -e 's#P=.*/tests/module_tests/#P=/tmp/vitess/tests/module_tests/#' \
           -e 's#P=.*/MODULE_TESTS/#P=/tmp/vitess/tests/module_tests/#' \
           -e 's#L=/tmp/.*\(........pipelog\)#L=/tmp/vitess\1#' \
           "${PIPELINE}"
}

main "$@"

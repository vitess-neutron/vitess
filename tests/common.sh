# detect msys/mingw
case "$(uname -s)" in
    MSYS*)
        WINDOWS=true
        ;;
    MINGW*)
        WINDOWS=true
        ;;
    *)
        ;;
esac

if [ "${WINDOWS}" = "true" ]; then
    [ -z "${TEMP_DIR}" ] && TEMP_DIR="${TESTS_DIR}/tmp"
    mkdir -p "${TEMP_DIR}" || exit 2
    SUFFIX=".exe"
    export SUFFIX
fi

# set directories relative to invoking script
[ -z "${TESTS_DIR}" ] && TESTS_DIR="$(cd "$(dirname "$0")" || exit 2; pwd)"
[ -z "${VITESS_DIR}" ] && VITESS_DIR="$(realpath "${TESTS_DIR}/..")"
[ -z "${V}" ] && V="${VITESS_DIR}/MODULES"
[ -z "${TEMP_DIR}" ] && TEMP_DIR="/tmp"

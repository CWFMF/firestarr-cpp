#!/bin/bash
IS_PASTED=
if [ "/bin/bash" == "$0" ]; then
  DIR_TEST=`realpath test`
  IS_PASTED=1
else
  set -e
  DIR_TEST="$(dirname $(realpath "$0"))"
fi
DIR_ROOT=$(dirname "${DIR_TEST}")
DIR_SUB=constant
DIR_OUT="${DIR_TEST}/data/${DIR_SUB}"


pushd ${DIR_ROOT}

VARIANT="$1"
if ( [ -z "${VARIANT}" ] || ( [ "Release" != "${VARIANT}" ] && [ "Debug" != ${VARIANT} ] && [ "Test" != "${VARIANT}" ]) ); then
  # assume that argument is an arg to pass to cmake
  VARIANT="Release"
  echo "${@}"
  # don't shift because $1 wasn't the variant
else
  shift;
fi

scripts/mk_clean.sh ${VARIANT}

HOURS="--hours 10"

rm -rf "${DIR_OUT}"
# mkdir -p "${DIR_OUT}"

# /usr/bin/time -v \
    ${DIR_ROOT}/firestarr test ${DIR_OUT} ${HOURS}
RET=$?
RESULT=0
if [ "0" -ne "${RET}" ]; then
  echo "Error during execution"
else
  # HACK: keep files in the same place as later in history
  FIX="C2_S000_A000_WD180.0_WS020.0"
  pushd "${DIR_OUT}"
  # check for directory so script stays the same through history
  if [ ! -d "${FIX}" ]; then
    mkdir "${FIX}"
    find . -type f -printf '%P\n' | xargs -I {} mv {} "${FIX}/${FIX}_{}"
    # ${DIR_OUT}/C2_S000_A000_WD180.0_WS020.0_arrival.tif
  fi
  git diff "${DIR_OUT}"
  RESULT=$?
fi

popd

popd
if [ "" == "${IS_PASTED}" ]; then
  exit ${RESULT}
fi

#!/bin/bash
HOURS="1"

IS_PASTED=
if [[ "$0" =~ "/bash" ]]; then
  DIR_TEST=`realpath test`
  IS_PASTED=1
else
  set -e
  DIR_TEST="$(dirname $(realpath "$0"))"
fi
DIR_ROOT=$(dirname "${DIR_TEST}")
DIR_SUB=constant
DIR_IN="${DIR_TEST}/input/${DIR_SUB}"
DIR_OUT="${DIR_TEST}/output/${DIR_SUB}"
# HACK: specifically use this so other constant outputs don't get deleted
DIR_OUT_SUB="${DIR_TEST}/output/${DIR_SUB}/C2_S000_A000_WD180.0_WS020.0"

# make it so release is required if anything is specified

VARIANT="$1"
if ( [ ! -z "${VARIANT}" ] ); then
  shift;
fi
# VAR_RELEASE="Release"
# VAR_DEBUG="Debug"
# VAR_TEST="Test"
# VAR_RELWITHDEBINFO="RelWithDebInfo"

# VARIANT="$1"
# if ( [ -z "${VARIANT}" ] || \
#     ( [ "${VAR_RELEASE,,}" != "${VARIANT,,}" ] \
#       && [ "${VAR_DEBUG,,}" != "${VARIANT,,}" ] \
#       && [ "${VAR_TEST,,}" != "${VARIANT,,}" ] \
#       && [ "${VAR_RELWITHDEBINFO,,}" != "${VARIANT,,}" ] \
#     ) ); then
#   # assume that argument is an arg to pass to cmake
#   VARIANT=${VAR_RELEASE}
#   # don't shift because $1 wasn't the variant
# else
#   shift;
# fi

pushd ${DIR_ROOT}

scripts/build.sh ${VARIANT}

rm -rf "${DIR_OUT_SUB}"
mkdir -p "${DIR_OUT}"

opts="--ascii"

# /usr/bin/time -v \
    ${DIR_ROOT}/firestarr test "${DIR_OUT}" --hours ${HOURS} ${opts}
RET=$?
RESULT=0
if [ "0" -ne "${RET}" ]; then
  echo "Error during execution"
else
  # # HACK: keep files in the same place as later in history
  # FIX="C2_S000_A000_WD180.0_WS020.0"
  # pushd "${DIR_OUT}"
  # # check for directory so script stays the same through history
  # if [ ! -d "${FIX}" ]; then
  #   mkdir "${FIX}"
  #   find . -type f -printf '%P\n' | xargs -I {} mv {} "${FIX}/${FIX}_{}"
  #   # ${DIR_OUT}/C2_S000_A000_WD180.0_WS020.0_arrival.tif
  # fi
  TEST_FOLDER=$(ls -d1 ${DIR_OUT})
  if [ $(echo "${TEST_FILES}" | wc -l) -ne 1 ]; then
      echo "ERROR: EXPECTED EXACTLY ONE TEST FOLDER"
      exit -1
  fi
  COMPARE_TO="HEAD~1"
  if [ "$(git log --oneline | wc -l)" -eq 1 ]; then
      COMPARE_TO="HEAD"
  fi
  GIT_PAGER=cat git diff ${COMPARE_TO} --name-status "${TEST_FOLDER}" 2>&1 > /dev/null
  RESULT=$?
  # if [ 0 -eq ${RESULT} ]; then
  #   # restore other changes since no changes to this test
  #   git restore ${DIR_OUT}
  # fi
fi

popd

if [ "" == "${IS_PASTED}" ]; then
  exit ${RESULT}
fi

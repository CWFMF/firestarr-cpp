#!/bin/bash
IS_PASTED=
if [[ "$0" =~ "/bash" ]]; then
  DIR_TEST=`realpath test`
  IS_PASTED=1
else
  set -e
  DIR_TEST="$(dirname $(realpath "$0"))"
fi
DIR_ROOT=$(dirname "${DIR_TEST}")
DIR_SUB=10N_50651
DIR_IN="${DIR_TEST}/input/${DIR_SUB}"
DIR_OUT="${DIR_TEST}/output/${DIR_SUB}"

BIN=$(realpath "${DIR_ROOT}/firestarr")
echo ${BIN}

pushd ${DIR_ROOT}
git restore settings.ini

VARIANT="$1"
if ( [ -z "${VARIANT}" ] || ( [ "Release" != "${VARIANT}" ] && [ "Debug" != ${VARIANT} ] && [ "Test" != "${VARIANT}" ]) ); then
  # assume that argument is an arg to pass to cmake
  VARIANT="Release"
  echo "${@}"
  # don't shift because $1 wasn't the variant
else
  shift;
fi

DAYS="$1"
if ( [ -z "${DAYS}" ] || ( [[ "${DAYS}" != +([0-9]) ]] ) ); then
  # assume that argument is an arg to pass to cmake
  DAYS=1
  echo "${@}"
  # don't shift because $1 wasn't the variant
else
  shift;
fi

echo "DAYS=${DAYS}"

# USE_TIME=
# if [ "Release" == "${VARIANT}" ]; then
  USE_TIME="/usr/bin/time -v"
# fi

opts=""
# opts="--sim-area"
intensity=""
# intensity="-i"
# intensity="--no-intensity"

scripts/build.sh ${VARIANT}

# make sure it only runs 1 sim each
sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 0/g" settings.ini
sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 0/g" settings.ini

#################

dates="[$(seq -s, ${DAYS})]"

rm -rf "${DIR_OUT}"
mkdir -p "${DIR_OUT}"
pushd "${DIR_OUT}"
FILE_WX="${DIR_IN}/firestarr_10N_50651_wx.csv"
FILE_PERIM="${DIR_IN}/10N_50651.tif"

${USE_TIME} \
  "${BIN}" . 2024-06-03 58.81228184403946 -122.9117103995713 01:00 \
    ${intensity} ${opts} \
    --ffmc 89.9 \
    --dmc 59.5 \
    --dc 450.9 \
    --apcp_prev 0 \
    -v \
    --wx "${FILE_WX}" \
    --output_date_offsets "${dates}" \
    --tz -7 \
    --perim "${FILE_PERIM}" ${@}
RET=$?
RESULT=0
if [ "0" -ne "${RET}" ]; then
  echo "Error during execution"
else
  find "${DIR_OUT}" -name "interim*" -delete
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

  tail -n1 "${DIR_OUT}/firestarr.log"
fi

popd
git restore settings.ini

popd
if [ "" == "${IS_PASTED}" ]; then
  exit ${RESULT}
fi

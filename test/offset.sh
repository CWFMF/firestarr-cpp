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
DIR_SUB=offset
DIR_IN="${DIR_TEST}/input/${DIR_SUB}"
DIR_OUT="${DIR_TEST}/output/${DIR_SUB}"

BIN=$(realpath "${DIR_ROOT}/firestarr")
echo ${BIN}

pushd ${DIR_ROOT}
# git restore settings.ini

VARIANT="$1"
if ( [ -z "${VARIANT}" ] \
  || ( [ "Release" != "${VARIANT}" ] \
    && [ "RelWithDebInfo" != ${VARIANT} ] \
    && [ "Debug" != ${VARIANT} ] \
    && [ "Test" != "${VARIANT}" ]) ); then
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

echo "DAYS=$DAYS"

OPTS=""
while [[ "$1" =~ -D* ]]
do
  OPTS="${OPTS} $1"
  shift;
done
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

# # make sure it only runs 1 sim each
# sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 0/g" settings.ini
# sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 0/g" settings.ini

#################

dates="[$(seq -s, ${DAYS})]"

rm -rf "${DIR_OUT}"
FILE_WX="${DIR_IN}/weather.csv"
FILE_PERIM="${DIR_IN}/perimeter.tif"

${USE_TIME} \
  "${BIN}" \
    "${DIR_OUT}" \
    2023-06-19 \
    60.82328538629729 \
    -115.70484253475514 \
    15:00 \
    --tz -8 \
    --wx "${FILE_WX}" \
    --ffmc 83.5 \
    --dmc 53.7 \
    --dc 568.9 \
    --apcp_prev 0 \
    --raster-root "${DIR_IN}" \
    --output_date_offsets ${dates} \
    --sim-area \
    --deterministic \
    --perim "${FILE_PERIM}" \
    -i
    ${OPTS} \
    ${@}

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

# git restore settings.ini

popd
if [ "" == "${IS_PASTED}" ]; then
  exit ${RESULT}
fi

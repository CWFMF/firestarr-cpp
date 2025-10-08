#!/bin/bash
# mixed-up version of a few tests to get a perimeter test from the start
IS_PASTED=
if [[ "$0" =~ "/bash" ]]; then
  DIR_TEST=`realpath test`
  IS_PASTED=1
else
  set -e
  DIR_TEST="$(dirname $(realpath "$0"))"
fi
DIR_ROOT=$(dirname "${DIR_TEST}")
DIR_OUT="${DIR_TEST}/output/perim"

FILE_PERIM="${DIR_TEST}/input/10N_50651/10N_50651.tif"
FILE_WX="${DIR_TEST}/input/hourly/wx_daily_in.csv"

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

opts="--ascii"
# opts="--sim-area"
# intensity=""
intensity="-i"
# intensity="--no-intensity"


scripts/build.sh ${VARIANT}

# make sure it only runs 1 sim each
sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 0/g" settings.ini
sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 0/g" settings.ini

#################

dates="[$(seq -s, ${DAYS})]"


rm -rf "${DIR_OUT}"
mkdir -p "${DIR_OUT}"

${USE_TIME} \
"${BIN}" "${DIR_OUT}" \
    2017-08-27 \
    58.81228184403946 \
    -122.9117103995713 \
    12:15 \
    ${intensity} ${opts} \
    --ffmc 90 \
    --dmc 40 \
    --dc 300 \
    --apcp_prev 0 \
    -v \
    --wx "${FILE_WX}" \
    --tz -5 \
    --perim "${FILE_PERIM}" ${@}
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
  # if [ 0 -eq ${RESULT} ]; then
  #   # restore other changes since no changes to this test
  #   git restore ${DIR_OUT}
  # fi
fi

popd
git restore settings.ini

if [ "" == "${IS_PASTED}" ]; then
  exit ${RESULT}
fi

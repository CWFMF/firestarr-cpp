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
DIR_SUB=10N_50651
DIR_OUT="${DIR_TEST}/data/${DIR_SUB}"


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

# USE_TIME=
# if [ "Release" == "${VARIANT}" ]; then
  USE_TIME="/usr/bin/time -v"
# fi

opts=""
# opts="--sim-area"
# intensity=""
intensity="-i"
# intensity="--no-intensity"

DAYS=14

scripts/build.sh ${VARIANT}

# make sure it only runs 21 sims
sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 0/g" settings.ini
sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 0/g" settings.ini

#################

dates="[$(seq -s, ${DAYS})]"

FILE_WX="${DIR_OUT}/firestarr_10N_50651_wx.csv"
FILE_PERIM="${DIR_OUT}/10N_50651.tif"
find -type f "${DIR_OUT}" \
  | grep -v "${FILE_WX}" \
  | grep -v "${FILE_PERIM}" \
  | xargs -tI {} rm

${USE_TIME} \
  ./firestarr ${DIR_OUT} 2024-06-03 58.81228184403946 -122.9117103995713 01:00 \
    ${intensity} ${opts} \
    --ffmc 89.9 \
    --dmc 59.5 \
    --dc 450.9 \
    --apcp_0800 0 \
    -v \
    --wx "${FILE_WX}" \
    --perim "${FILE_PERIM}" $*
RET=$?
RESULT=0
if [ "0" -ne "${RET}" ]; then
  echo "Error during execution"
else
  find ${DIR_OUT} -type f -name "interim*" | xargs -I {} rm {}
  # debug mode dumps weather files that we can't compare to release mode
  find ${DIR_OUT} -type f -name "*wx_*_out*" | xargs -I {} rm {}
  find ${DIR_OUT} -type f -name "*.aux.xml" | xargs -I {} rm {}

  if [ -d "${DIR_ORIG}" ]; then
    # use echo to not quit if diff fails
    # HACK: use && instead of || because if nothing exists after grep then no differences
    ( \
      diff --exclude="*.log" -rq "${DIR_ORIG}" "${DIR_OUT}" \
        | grep -v "^Only in ${DIR_ORIG}:" \
    ) \
    && \
    ( \
      echo "Different results" \
        && RESULT=1
    )
else
    echo "Outputs did not exist so copying to ${DIR_ORIG}"
    cp -r "${DIR_OUT}" "${DIR_ORIG}"
    rm -f "${DIR_ORIG}/*.log"
  fi
  tail -n1 "${DIR_OUT}/firestarr.log"
fi

popd
git restore settings.ini

popd
if [ "" == "${IS_PASTED}" ]; then
  exit ${RESULT}
fi

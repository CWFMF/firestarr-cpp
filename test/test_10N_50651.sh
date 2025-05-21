#!/bin/bash
IS_PASTED=
if [ "/bin/bash" == "$0" ]; then
  DIR_ROOT=`realpath test`
  IS_PASTED=1
else
  set -e
  DIR_ROOT="$(dirname $(realpath "$0"))"
fi
DIR="${DIR_ROOT}/10N_50651"


pushd /appl/firestarr
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
intensity=""
# intensity="-i"
# intensity="--no-intensity"

DAYS="$1"
if [ "" == "${DAYS}" ]; then
  DAYS=3
else
  if [ ! "${DAYS}" -gt 0 ] || [ ! "${DAYS}" -le 14 ]; then
    echo "Number of days must be an integer between 1 and 14 inclusive but got: ${DAYS} so passing to test instead"
    DAYS=3
  else
    # get rid of parameter
    shift;
  fi
fi

scripts/build.sh ${VARIANT}

# make sure it only runs 21 sims
sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 0/g" settings.ini
sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 0/g" settings.ini

#################
pushd "${DIR}"

dates="[$(seq -s, ${DAYS})]"
dir_out="${dates}/"

rm -rf "${dir_out}"
mkdir -p "${dir_out}"

${USE_TIME} \
  ../../firestarr ./${dir_out} 2024-06-03 58.81228184403946 -122.9117103995713 01:00 ${intensity} ${opts} --ffmc 89.9 --dmc 59.5 --dc 450.9 --apcp_prev 0 -v --output_date_offsets ${dates} --wx firestarr_10N_50651_wx.csv --perim 10N_50651.tif $*
RET=$?
RESULT=0
if [ "0" -ne "${RET}" ]; then
  echo "Error during execution"
else
  find ${DIR} -type f -name "interim*" | xargs -I {} rm {}
  # debug mode dumps weather files that we can't compare to release mode
  find ${DIR} -type f -name "*wx_*_out*" | xargs -I {} rm {}
  find ${DIR} -type f -name "*.aux.xml" | xargs -I {} rm {}
  dir_orig="${DIR}.orig"

  if [ -d ${dir_orig} ]; then
    # pushd "${dir_orig}"
    # rm -f "*.xml"
    # popd
    # use echo to not quit if diff fails
    # HACK: use && instead of || because if nothing exists after grep then no differences
    ( \
      diff --exclude="*.log" -rq "${dir_orig}" "${dir_out}" \
        | grep -v "^Only in ${dir_orig}:" \
    ) \
    && \
    ( \
      echo "Different results" \
        && RESULT=1
    )
else
    echo "Outputs did not exist so copying to ${dir_orig}"
    cp -r "${dir_out}" "${dir_orig}"
    rm -f "${dir_orig}/*.log"
  fi
  tail -n1 "${dir_out}/firestarr.log"
fi

popd
git restore settings.ini

popd
if [ "" == "${IS_PASTED}" ]; then
  exit ${RESULT}
fi

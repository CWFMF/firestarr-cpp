#!/bin/bash
TIME_LIMIT=300

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
DIR_FLAMEGRAPHS=${DIR_ROOT}/flamegraphs

BIN=$(realpath "${DIR_ROOT}/firestarr")
echo ${BIN}

pushd ${DIR_ROOT}
git restore settings.ini

REV=`git log --oneline -1 | sed "s/ .*//g"`
MSG=`git log --oneline -1 | sed "s/[^ ]* \(.*\)/\1/g"`
NUM=`git log --oneline | wc -l`
NUM_PADDED=`printf "%04g" $(($NUM + 0))`

echo $REV
echo $MSG

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

# make sure it only runs 1 sim each
sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 0/g" settings.ini
sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 0/g" settings.ini

#################

dates="[$(seq -s, ${DAYS})]"
DAYS_PADDED=`printf '%02g' ${DAYS}`
name_out="fg_${NUM_PADDED}_${REV}_${DAYS_PADDED}_days"

rm -rf "${DIR_OUT}"
mkdir -p "${DIR_OUT}"
pushd "${DIR_OUT}"
FILE_WX="${DIR_IN}/firestarr_10N_50651_wx.csv"
FILE_PERIM="${DIR_IN}/10N_50651.tif"


echo "Running profile"

# HACK: kill if running for too long
((sleep ${TIME_LIMIT} && killall firestarr) > /dev/null 2>&1) &

# HACK: if finishes before sleep does then stop sleep so it doesn't affect next run
${DIR_ROOT}/scripts/profile.sh \
    "${BIN}" . 2024-06-03 58.81228184403946 -122.9117103995713 01:00 \
    ${intensity} ${opts} \
    --ffmc 89.9 \
    --dmc 59.5 \
    --dc 450.9 \
    --apcp_prev 0 \
    -v \
    --wx "${FILE_WX}" \
    --output_date_offsets "${dates}" \
    --tz -5 \
    --perim "${FILE_PERIM}" ${@} \
  && (killall sleep > /dev/null 2>&1 && echo "Ran within ${TIME_LIMIT}s time limit")

duration=`grep "Total simulation time" ${DIR_OUT}/firestarr.log | sed "s/.* was \(.*\) seconds/\1/"`

mkdir -p "${DIR_FLAMEGRAPHS}"
sed -i "s/Flame Graph/${NUM_PADDED} - ${REV} - ${DAYS} days - ${duration}s/" flame.html
sed -i "/id=\"details\"/{s/> </>${MSG//\//\\\/}</g}" flame.html
# just copies into directory if no argument but renames if there is one
file_out="${DIR_FLAMEGRAPHS}/${name_out}.html"
mv flame.html "${file_out}"
popd
git restore settings.ini
popd

echo "Profile saved as ${file_out}"

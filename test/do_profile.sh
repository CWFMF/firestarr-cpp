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
DIR_FLAMEGRAPHS="${DIR_TEST}/flamegraphs"
DIR="${DIR_TEST}/10N_50651"
TIME_LIMIT=300

REV=`git log --oneline -1 | sed "s/ .*//g"`
MSG=`git log --oneline -1 | sed "s/[^ ]* \(.*\)/\1/g"`
NUM=`git log --oneline | wc -l`
NUM_PADDED=`printf "%04g" $(($NUM + 0))`

echo $REV
echo $MSG

opts="--ascii"
intensity=""
# intensity="-i"
# intensity="--no-intensity"

DAYS="$1"
if [ "" == "${DAYS}" ]; then
  DAYS=7
else
  if [ ! "${DAYS}" -gt 0 ] || [ ! "${DAYS}" -le 14 ]; then
    echo "Number of days must be an integer between 1 and 14 inclusive but got: ${DAYS}"
    exit
  fi
fi
dates="[$(seq -s, ${DAYS})]"
dir_out="${dates}/"
DAYS_PADDED=`printf '%02g' ${DAYS}`
name_out="fg_${NUM_PADDED}_${REV}_${DAYS_PADDED}_days"

pushd ${DIR}

rm -rf "${dir_out}"
mkdir -p "${dir_out}"
rm -f "${DIR_ROOT}/firestarr"

echo "Running test"
# HACK: make sure test doesn't change output files
`ls -1 ${DIR_TEST}/test*.sh` 1 > /dev/null 2>&1 || (echo "Test failed" && exit 1)


pushd /appl/firestarr
git restore settings.ini
# make sure it only runs 21 sims
sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 0/g" settings.ini
sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 0/g" settings.ini
popd

echo "Running profile"

# HACK: kill if running for too long
((sleep ${TIME_LIMIT} && killall firestarr) > /dev/null 2>&1) &

# HACK: if finishes before sleep does then stop sleep so it doesn't affect next run
${DIR_ROOT}/scripts/profile.sh \
    ${DIR_ROOT}/firestarr ${dir_out} 2024-06-03 58.81228184403946 -122.9117103995713 \
      01:00 ${intensity} ${opts} --ffmc 89.9 --dmc 59.5 --dc 450.9 --apcp_prev 0 \
      -v --output_date_offsets ${dates} --wx firestarr_10N_50651_wx.csv --perim 10N_50651.tif \
    && (killall sleep > /dev/null 2>&1 && echo "Ran within ${TIME_LIMIT}s time limit")


pushd /appl/firestarr
git restore settings.ini
popd

duration=`grep "Total simulation time" ${dir_out}/firestarr.log | sed "s/.* was \(.*\) seconds/\1/"`

mkdir -p "${DIR_FLAMEGRAPHS}"
sed -i "s/Flame Graph/${NUM_PADDED} - ${REV} - ${DAYS} days - ${duration}s/" flame.html
sed -i "/id=\"details\"/{s/> </>${MSG//\//\\\/}</g}" flame.html
# just copies into directory if no argument but renames if there is one
file_out="${DIR_FLAMEGRAPHS}/${name_out}.html"
mv flame.html "${file_out}"
popd
echo "Profile saved as ${file_out}"

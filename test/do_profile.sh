#!/bin/bash
DIR_TEST="$(dirname $(realpath "$0"))"
DIR_FLAMEGRAPHS="${DIR_TEST}/flamegraphs"
DIR="${DIR_TEST}/10N_50651"

REV=`git log --oneline -1 | sed "s/ .*//g"`
MSG=`git log --oneline -1 | sed "s/[^ ]* \(.*\)/\1/g"`

echo $REV
echo $MSG
name_out="fg_${REV}"

opts=""
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

pushd ${DIR}

rm -rf "${dir_out}"
mkdir -p "${dir_out}"

/appl/firestarr/scripts/profile.sh \
    /appl/firestarr/firestarr ./${dir_out} 2024-06-03 58.81228184403946 -122.9117103995713 01:00 ${intensity} ${opts} --ffmc 89.9 --dmc 59.5 --dc 450.9 --apcp_prev 0 -v --output_date_offsets ${dates} --wx firestarr_10N_50651_wx.csv --perim 10N_50651.tif

duration=`grep "Total simulation time" ${dir_out}/firestarr.log | sed "s/.* was \(.*\) seconds/\1/"`

mkdir -p "${DIR_FLAMEGRAPHS}"
sed -i "s/Flame Graph/${REV} - ${duration}s/" flame.html
sed -i "/id=\"details\"/{s/> </>${MSG}</g}" flame.html
# just copies into directory if no argument but renames if there is one
mv flame.html "${DIR_FLAMEGRAPHS}/${name_out}.html"
popd

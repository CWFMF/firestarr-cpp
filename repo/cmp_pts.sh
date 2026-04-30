#!/bin/bash

DAYS="$1"
if ( [ -z "${DAYS}" ] || ( [[ "${DAYS}" != +([0-9]) ]] ) ); then
  # assume that argument is an arg to pass to cmake
  DAYS=1
  echo "${@}"
  # don't shift because $1 wasn't the variant
else
  shift;
fi

dates="[$(seq -s, ${DAYS})]"

d_fmt=$(printf "%03d" ${DAYS})
DIR_CMP=cmp
dir_cur=${DIR_CMP}/${d_fmt}
mkdir -p ${dir_cur}
log=${DIR_CMP}/times_${d_fmt}.log
rm "${log}"

git restore src
check=""
# doing even 4 points makes no sense, but try it
for n in $(seq 1 360)
do
  if [[ $(expr 360 % ${n}) -eq 0 ]]; then
    check="${check} ${n}"
  fi
done
echo "Should check: ${check}"
for N in old ${check}
do
  if [[ "old" == "$N" ]]; then
    dir=old
    git restore -s 4ccfb040c3 src
  else
    dir=${N}pt
    git restore src
    sed -i "s/\(constexpr auto N = \).*;/\1${N};/" src/cpp/fs/SpreadAlgorithm.cpp
  fi
  echo "#######################################################################" | tee -a "${log}"
  echo "${dir}" | tee -a "${log}"
  echo "#######################################################################" | tee -a "${log}"
  scripts/build.sh > /dev/null 2>&1
  /usr/bin/time -v \
    /appl/firestarr/firestarr ${dir_cur}/${dir} 2024-06-03 \
    58.81228184403946 -122.9117103995713 01:00 \
    --ffmc 89.9 --dmc 59.5 --dc 450.9 --apcp_prev 0 \
    -v \
    --wx /appl/firestarr/test/input/10N_50651/firestarr_10N_50651_wx.csv \
    --output_date_offsets "${dates}" \
    --tz -5 \
    --raster-root /appl/firestarr/test/input/10N_50651/ \
    --perim /appl/firestarr/test/input/10N_50651/10N_50651.tif \
    --deterministic \
    > ${DIR_CMP}/cur.log 2>&1
  # only want the last part of the log from "Ran <N> simulations"
  sed -n "/Ran .* simulations$/,/Exit status:.*/{p}" ${DIR_CMP}/cur.log | tee -a "${log}"
  # tail -n 28 ${DIR_CMP}/cur.log >> "${log}"
done

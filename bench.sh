#!/bin/bash
set -e
set -o pipefail
n=$(printf "%03d" $1)
echo $n
DAYS=$2
if [[ -z "${DAYS}" ]]; then
  DAYS=10
fi
D=benchmark$(printf "%03d" ${DAYS})
d="out/${D}/${n}"
echo $d
# exit
mkdir -p ${d} \
  && (/appl/firestarr/fs_release \
      ${d} \
      2024-06-03 \
      58.81228184403946 \
      -122.9117103995713 \
      01:00 \
      --ffmc 89.9 \
      --dmc 59.5 \
      --dc 450.9 \
      --apcp_prev 0 \
      -v \
      --wx /appl/firestarr/test/input/10N_50651/firestarr_10N_50651_wx.csv \
      --output_date_offsets [$(seq -s, $DAYS)] \
      --tz -5 \
      --raster-root /appl/firestarr/test/input/10N_50651/ \
      --perim /appl/firestarr/test/input/10N_50651/10N_50651.tif \
      --no-intensity \
  && diff -rq -x "*.log" -x "*.ini" out/${D}/000/ ${d} \
  && echo "Results are the same" \
) | tee ${d}/out.log 2>&1

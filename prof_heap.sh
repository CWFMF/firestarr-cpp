#!/bin/bash
DAYS="$1"
if ( [ -z "${DAYS}" ] || ( [[ "${DAYS}" != +([0-9]) ]] ) ); then
  # assume that argument is an arg to pass to cmake
  DAYS=1
  echo "${@}"
else
  shift;
fi

echo "DAYS=${DAYS}"

# make sure it only runs 1 sim each
sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 0/g" settings.ini
sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 0/g" settings.ini

#################

dates="[$(seq -s, ${DAYS})]"

mkdir -p /appl/firestarr/test/output/10N_50651
mkdir -p /appl/firestarr/profile/heap
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
HEAPPROFILE=profile/heap/fs.hprof \
HEAP_PROFILE_ALLOCATION_INTERVAL=104857600 \
  /appl/firestarr/firestarr \
  /appl/firestarr/test/output/10N_50651 \
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
  --output_date_offsets ${dates} \
  --tz -5 \
  --perim /appl/firestarr/test/input/10N_50651/10N_50651.tif

git restore settings.ini

google-pprof --text /appl/firestarr/firestarr $(find profile/heap -type f | tail -n1) | head -n20

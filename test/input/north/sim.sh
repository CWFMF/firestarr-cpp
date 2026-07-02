#!/bin/bash
# DIR=$(pwd)
DIR="$(dirname $(realpath "$0"))"

DEFAULT_DAYS=7


DAYS="$1"
if ( [ -z "${DAYS}" ] || ( [[ "${DAYS}" != +([0-9]) ]] ) ); then
  # assume that argument is an arg to pass to cmake
  DAYS=${DEFAULT_DAYS}
  echo "${@}"
  # don't shift because $1 wasn't the variant
else
  shift;
fi

DIR_OUT="${DIR}/day_${DAYS}"
DATES="[$(seq -s, ${DAYS})]"
OPTS=""
# OPTS="-i -v"

mkdir -p "${DIR_OUT}"
/appl/firestarr/firestarr \
  "${DIR_OUT}" \
  2023-06-19 \
  60.82328538629729 \
  -115.70484253475514 \
  19:00 \
  --tz \
  -8 \
  --wx \
  ${DIR}/weather.csv \
  --ffmc \
  83.5 \
  --dmc \
  53.7 \
  --dc \
  568.9 \
  --apcp_prev \
  0 \
  --perim \
  ${DIR}/perimeter.tif \
  --output_date_offsets \
  ${DATES} \
  --sim-area \
  ${OPTS} \
  ${@}
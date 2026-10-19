#!/bin/bash
set -e
# system timezone should not change anything about simulations since they always expect LST
RESULT=0
# test all timezones listed in Canada
for tz in \
  "UTC" \
  "America/Cambridge_Bay" \
  "America/Creston" \
  "America/Dawson" \
  "America/Dawson_Creek" \
  "America/Fort_Nelson" \
  "America/Inuvik" \
  "America/Vancouver" \
  "America/Whitehorse" \
  "America/Edmonton" \
  "America/Rankin_Inlet" \
  "America/Regina" \
  "America/Resolute" \
  "America/Swift_Current" \
  "America/Winnipeg" \
  "America/Atikokan" \
  "America/Iqaluit" \
  "America/Toronto" \
  "America/Blanc-Sablon" \
  "America/Glace_Bay" \
  "America/Goose_Bay" \
  "America/Halifax" \
  "America/Moncton" \
  "America/St_Johns" \
  ;
do
  DIR_OUT="test/output/$tz"
  rm -rf "${DIR_OUT}"
  mkdir -p "${DIR_OUT}"
  (TZ=$tz \
    ./firestarr \
    "${DIR_OUT}" \
    2024-06-03 \
    58.81228184403946 -122.9117103995713 \
    01:00 \
    --ffmc 89.9 \
    --dmc 59.5 \
    --dc 450.9 \
    --apcp_prev 0 \
    -v \
    --wx /appl/firestarr/test/input/10N_50651/firestarr_10N_50651_wx.csv \
    --output_date_offsets [1] \
    --tz -5 \
    --raster-root /appl/firestarr/test/input/10N_50651/ \
    --perim /appl/firestarr/test/input/10N_50651/10N_50651.tif \
    --no-intensity \
    -v \
    > /dev/null)
  grep "TIME" "${DIR_OUT}/settings.ini" > "${DIR_OUT}/settings.log"
  diff "test/output/UTC/settings.log" "${DIR_OUT}/settings.log" > /dev/null && echo "Same for $tz"
  R=$?
  if [ "$R" -ne 0 ]; then
    echo -e "Different for $tz:"
    (diff test/output/UTC/settings.log "${DIR_OUT}/settings.log" || ([ "$RESULT" -eq 0 ] && RESULT=$R)) | sed "s/^/    /g"
  fi
done
exit $RESULT

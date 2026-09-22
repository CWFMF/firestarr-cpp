#!/bin/bash
set -e
FILE_WX=test/output/csv/header_casing.csv
DIR_OUT=test/output/csv
scripts/build.sh
for h in \
  "ScEnArio,date,PreC,TeMP,RH,ws,Wd,FFmC,DMC,DC,ISI,BUI,FWI" \
  "SCENARIO,DATE,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI" \
  "scenario,date,prec,temp,rh,ws,wd,ffmc,dmc,dc,isi,bui,fwi" \
  "scen,date,prec,temp,rh,ws,wd,ffmc,dmc,dc,isi,bui,fwi" \
  ;
do
  echo -e "Using header:\n${h}"
  rm -rf "${DIR_OUT}"
  mkdir -p "${DIR_OUT}"
  echo ${h} > ${FILE_WX}
  grep -E "^0," test/input/10N_50651/firestarr_10N_50651_wx.csv >> ${FILE_WX}
  ./firestarr \
    "${DIR_OUT}" \
    2024-06-03 59 -123 01:00 \
    --ffmc 89.9 \
    --dmc 59.5 \
    --dc 450.9 \
    --apcp_prev 0 \
    --no-probability \
    -v \
    -i \
    -s \
    --deterministic \
    --raster-root test/input/10N_50651 \
    --wx ${FILE_WX} \
    --output_date_offsets [1] \
    --tz -7 \
    $@
done

#!/bin/bash
TYPE=ZSTD

OPTS=""
OPTS="${OPTS} --co=TILED=YES"
OPTS="${OPTS} --co=BLOCKXSIZE=4096"
OPTS="${OPTS} --co=BLOCKYSIZE=4096"
OPTS="${OPTS} --co=COMPRESS=${TYPE}"
OPTS="${OPTS} --co=PREDICTOR=2"
OPTS="${OPTS} --co=ZLEVEL=9"

DIR_IN="../../../data/generated/grid/100m/2023"

echo ${TYPE}
ls -1 ${DIR_IN}/dem* \
  | sed "s/.*\/\([^\/]*\)/\1/g" \
  | xargs -tI{} \
    gdal_calc.py --calc=100 --outfile=./{} --NoDataValue=0 -A ${DIR_IN}/{} ${OPTS} --overwrite

ls -1 ${DIR_IN}/fuel* \
  | sed "s/.*\/\([^\/]*\)/\1/g" \
  | xargs -tI{} \
    gdal_calc.py --calc=2 --outfile=./{} --NoDataValue=0 -A ${DIR_IN}/{} ${OPTS} --overwrite

# gdal_calc.py --calc=100 --outfile=./dem_test.tif --NoDataValue=0 -A ../../../../data/generated/grid/100m/2023/dem_11_0.tif ${OPTS} --overwrite
# gdal_calc.py --calc=2 --outfile=./fuel_test.tif --NoDataValue=0 -A ../../../../data/generated/grid/100m/2023/fuel_11_0.tif ${OPTS} --overwrite
ls -lh *.tif

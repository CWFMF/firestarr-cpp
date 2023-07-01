#!/bin/bash
URL_TEST="https://spotwx.com/products/grib_index.php?model=geps_0p5_raw&lat=48.80686&lon=-87.45117&tz=-5&label="
DIR=/appl/data
PREFIX=geps
FILE_LATEST=${DIR}/${PREFIX}_latest
FILE_CURRENT=${DIR}/${PREFIX}_current
FILE_TMP=${DIR}/${PREFIX}_tmp
echo Running update at `date`
source /appl/.venv/bin/activate
cd /appl/tbd
# copy after trying instead of going right to ${FILE_LATEST} in case curl fails and makes an empty file
((curl -sk "${URL_TEST}" | grep "Model date" > ${FILE_TMP}) \
        && (mv ${FILE_TMP} ${FILE_LATEST}) \
        && ((diff ${FILE_CURRENT} ${FILE_LATEST} \
                && echo Model already matches `cat ${FILE_LATEST}`) \
            || ((python main.py) \
                && (cp ${FILE_LATEST} ${FILE_CURRENT}) \
                && (./publish_geoserver.sh)))) \
    || (echo Run attempt failed)

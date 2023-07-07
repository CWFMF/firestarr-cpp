#!/bin/bash
URL_TEST="https://spotwx.com/products/grib_index.php?model=geps_0p5_raw&lat=48.80686&lon=-87.45117&tz=-5&label="
DIR=/appl/data
CURDATE=`date -u --rfc-3339=seconds`
PREFIX=geps
FILE_LATEST=${DIR}/${PREFIX}_latest
FILE_CURRENT=${DIR}/${PREFIX}_current
FILE_TMP=${DIR}/${PREFIX}_tmp
FILE_LOG=${DIR}/${PREFIX}_log
source /appl/.venv/bin/activate
cd /appl/tbd
# echo ${CURDATE} >> ${FILE_LOG}
# copy after trying instead of going right to ${FILE_LATEST} in case curl fails and makes an empty file
( \
    ( \
        (curl -sk "${URL_TEST}" | grep "Model date" > ${FILE_TMP}) \
        && (mv ${FILE_TMP} ${FILE_LATEST}) \
    ) \
    || ( \
        (echo ${CURDATE}: failed to get ${URL_TEST} | tee -a ${FILE_LOG}) \
        && (exit -1) \
    ) \
) \
&& ( \
    ( \
        diff ${FILE_CURRENT} ${FILE_LATEST} \
        && echo ${CURDATE}: Already up to date >> ${FILE_LOG}
    ) \
    || \
    ( \
        (echo ${CURDATE}: Running update  | tee -a ${FILE_LOG}) \
        && (python main.py) \
        && (cp ${FILE_LATEST} ${FILE_CURRENT}) \
        && (echo ${CURDATE}: Done update  | tee -a ${FILE_LOG}) \
    ) \
) \
|| (echo ${CURDATE}: Run attempt failed | tee -a ${FILE_LOG})

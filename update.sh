#!/bin/bash
. /appl/config

if [ -z "${SPOTWX_API_KEY}" ] \
    || [ -z "${BOUNDS_LATITUDE_MAX}" ] \
    || [ -z "${BOUNDS_LATITUDE_MIN}" ] \
    || [ -z "${BOUNDS_LONGITUDE_MAX}" ] \
    || [ -z "${BOUNDS_LONGITUDE_MIN}" ] \
    ; then
    echo SPOTWX_API_KEY must be set
else
    LATITUDE=$((${BOUNDS_LATITUDE_MAX} - ${BOUNDS_LATITUDE_MIN}))
    LONGITUDE=$((${BOUNDS_LONGITUDE_MAX} - ${BOUNDS_LONGITUDE_MIN}))
    MODEL=geps
    URL_TEST="https://spotwx.io/api.php?key=${SPOTWX_API_KEY}&lat=${LATITUDE}&lon=${LONGITUDE}&model=${MODEL}&output=archive"
    DIR=/appl/data
    CURDATE=`date -u --rfc-3339=seconds`
    FILE_LATEST=${DIR}/${MODEL}_latest
    FILE_CURRENT=${DIR}/${MODEL}_current
    FILE_TMP=${DIR}/${MODEL}_tmp
    FILE_LOG=${DIR}/${MODEL}_log
    source /appl/.venv/bin/activate
    cd /appl/tbd
    # echo ${CURDATE} >> ${FILE_LOG}
    # copy after trying instead of going right to ${FILE_LATEST} in case curl fails and makes an empty file
    ( \
        ( \
            (curl -sk "${URL_TEST}" -o ${FILE_TMP}) \
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
            && (python main.py $*) \
            && (cp ${FILE_LATEST} ${FILE_CURRENT}) \
            && (echo $(date -u --rfc-3339=seconds): Done update  | tee -a ${FILE_LOG}) \
        ) \
    ) \
    || (echo ${CURDATE}: Run attempt failed | tee -a ${FILE_LOG})
fi

#/bin/bash
DIR=`dirname $(realpath "$0")`
. ${DIR}/geoserver_setup.sh

if [ -z "${LAYER}" ] \
    || [ -z "${COVERAGE}" ] \
    || [ -z "${CREDENTIALS}" ] \
    || [ -z "${SERVER}" ] \
    || [ -z "${WORKSPACE_NAME}" ] \
    || [ -z "${DIR_DATA}" ] \
    ; then
    echo Missing required configuration so not publishing
else
    EXTENSION=imagemosaic
    WORKSPACE=${SERVER}/workspaces/${WORKSPACE_NAME}
    STORE=$WORKSPACE/coveragestores/${LAYER}
    echo "Publishing to ${STORE}"

    # # get rid of old granules
    # curl -v -v -sS -u "${CREDENTIALS}" -XDELETE "${STORE}/coverages/${COVERAGE}/index/granules.xml"
    # # update to match azure mount
    # curl -v -u "${CREDENTIALS}" -XPOST -H "Content-type: text/plain" --write-out %{http_code} -d "${DIR_DATA}" "${STORE}/external.${EXTENSION}"

    # # get run id from name of files
    # RUN_ID=`curl -v -v -sS -u "${CREDENTIALS}" -XGET "${STORE}/coverages/${COVERAGE}/index/granules.xml" | grep .tif | tail -n 1 | sed "s/.*firestarr_\([0-9]*\)_.*\.tif.*/\1/g"`
    # ABSTRACT="FireSTARR run from ${RUN_ID}"
    # # replace abstract
    # curl -v -v -sS -u "${CREDENTIALS}" -XGET "${STORE}/coverages/${COVERAGE}" | sed "s/<abstract>[^<]*<\/abstract>/<abstract>${ABSTRACT}<\/abstract>/g" > /tmp/${COVERAGE}.xml
    # # upload with updated abstract
    # curl -v -u "${CREDENTIALS}" -XPUT -H "Content-type: text/xml" -d @/tmp/${COVERAGE}.xml "${STORE}/coverages/${COVERAGE}"?calculate=nativebbox,latlonbbox,dimensions
fi

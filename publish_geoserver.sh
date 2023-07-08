#/bin/bash
DIR=`dirname $(realpath "$0")`
. /appl/config

if [ -z "${GEOSERVER_LAYER}" ] \
    || [ -z "${GEOSERVER_COVERAGE}" ] \
    || [ -z "${GEOSERVER_CREDENTIALS}" ] \
    || [ -z "${GEOSERVER_SERVER}" ] \
    || [ -z "${GEOSERVER_WORKSPACE_NAME}" ] \
    || [ -z "${GEOSERVER_DIR_DATA}" ] \
    ; then
    echo Missing required configuration so not publishing
else
    GEOSERVER_EXTENSION=imagemosaic
    GEOSERVER_WORKSPACE=${GEOSERVER_SERVER}/workspaces/${GEOSERVER_WORKSPACE_NAME}
    GEOSERVER_STORE=${GEOSERVER_WORKSPACE}/coveragestores/${GEOSERVER_LAYER}
    echo "Publishing to ${GEOSERVER_STORE}"

    # get rid of old granules
    curl -v -v -sS -u "${GEOSERVER_CREDENTIALS}" -XDELETE "${GEOSERVER_STORE}/coverages/${GEOSERVER_COVERAGE}/index/granules.xml"
    # update to match azure mount
    curl -v -u "${GEOSERVER_CREDENTIALS}" -XPOST -H "Content-type: text/plain" --write-out %{http_code} -d "${GEOSERVER_DIR_DATA}" "${GEOSERVER_STORE}/external.${GEOSERVER_EXTENSION}"

    # get run id from name of files
    RUN_ID=`curl -v -v -sS -u "${GEOSERVER_CREDENTIALS}" -XGET "${GEOSERVER_STORE}/coverages/${GEOSERVER_COVERAGE}/index/granules.xml" | grep .tif | tail -n 1 | sed "s/.*firestarr_\([0-9]*\)_.*\.tif.*/\1/g"`
    ABSTRACT="FireSTARR run from ${RUN_ID}"
    # replace abstract
    curl -v -v -sS -u "${GEOSERVER_CREDENTIALS}" -XGET "${GEOSERVER_STORE}/coverages/${GEOSERVER_COVERAGE}" | sed "s/<abstract>[^<]*<\/abstract>/<abstract>${ABSTRACT}<\/abstract>/g" > /tmp/${GEOSERVER_COVERAGE}.xml
    # upload with updated abstract
    curl -v -u "${GEOSERVER_CREDENTIALS}" -XPUT -H "Content-type: text/xml" -d @/tmp/${GEOSERVER_COVERAGE}.xml "${GEOSERVER_STORE}/coverages/${GEOSERVER_COVERAGE}"?calculate=nativebbox,latlonbbox,dimensions
fi

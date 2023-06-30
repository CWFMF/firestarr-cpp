#/bin/bash
# LAYER=firestarr_mount
# COVERAGE=FireSTARR
# EXTENSION=imagemosaic
# CREDENTIALS=
# SERVER=
# WORKSPACE=${SERVER}/workspaces/firestarr
# STORE=$WORKSPACE/coveragestores/${LAYER}

# # get rid of old granules
# curl -v -v -sS -u "${CREDENTIALS}" -XDELETE "${STORE}/coverages/${COVERAGE}/index/granules.xml"
# # update to match azure mount
# curl -v -u "${CREDENTIALS}" -XPOST -H "Content-type: text/plain" --write-out %{http_code} -d "/data/firestarr_data/current" "${STORE}/external.${EXTENSION}"

# # # HACK: know that .zip implies uploaded properly and has runid in name
# # RUN_ID=`ls -1 ../data/output/current_m3/*.zip | tail -n1 | sed "s/.*_\([0-9]*\)\.zip/\1/g"`
# # get run id from name of files
# RUN_ID=`curl -v -v -sS -u "${CREDENTIALS}" -XGET "${STORE}/coverages/${COVERAGE}/index/granules.xml" | grep .tif | tail -n 1 | sed "s/.*firestarr_\([0-9]*\)_.*\.tif.*/\1/g"`
# ABSTRACT=FireSTARR run from ${RUN_ID}
# # replace abstract
# curl -v -v -sS -u "${CREDENTIALS}" -XGET "${STORE}/coverages/${COVERAGE}" | sed "s/<abstract>[^<]*<\/abstract>/<abstract>${ABSTRACT}<\/abstract>/g" > ${COVERAGE}.xml
# # upload with updated abstract
# curl -v -u "${CREDENTIALS}" -XPUT -H "Content-type: text/xml" -d @${COVERAGE}.xml "${STORE}/coverages/${COVERAGE}"

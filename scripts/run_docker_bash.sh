#!/bin/bash
IMAGE=tbd_prod_stable
# REGISTRY=registrycwfisdev.azurecr.io/firestarr
REGISTRY=ghcr.io/jordan-evens
DIR_DATA=/mnt/batch/tasks/fsmounts/firestarr_data
CONTAINER=`docker ps | grep ${IMAGE} | sed "s/.* \([^ ]*$\)/\1/g"`
DIR_CONTAINER=${DIR_DATA}/container
# # this is wrong because we're running it outside the container
# FILE_SCRIPT=`$(realpath "$0")`
# echo Syncing ${FILE_SCRIPT} to ${DIR_CONTAINER}
# cp "${FILE_SCRIPT}" "${DIR_CONTAINER}/"
if [ -z "${CONTAINER}" ]; then
  echo Nothing running
  CONTAINER=`docker ps -a | grep ${IMAGE} | head -n 1 | sed "s/.* \([^ ]*$\)/\1/g"`
    if [ -z "${CONTAINER}" ]; then
        echo No existing container so making new one
        USE_IMAGE=${REGISTRY}/${IMAGE}:latest
    else
        echo Restarting based on ${CONTAINER}
        # not running so need to save and start
        USE_IMAGE=${IMAGE}:latest
        docker commit ${CONTAINER} ${USE_IMAGE}
        CUR_DATE=`date +"%Y%m%d_%H%M"`
        FILE_BKUP="${DIR_CONTAINER}/${IMAGE}_${CUR_DATE}.tar"
        echo Saving to ${FILE_BKUP}
        docker save -o "${FILE_BKUP}" ${IMAGE}
        echo Restore this image with "docker load -i ${FILE_BKUP}"
    fi
    docker run -it --entrypoint /bin/bash --workdir /appl/tbd -v ${DIR_DATA}:/appl/data ${USE_IMAGE}
else
  echo Attaching to running container ${CONTAINER}
  docker exec -it ${CONTAINER} /bin/bash
fi

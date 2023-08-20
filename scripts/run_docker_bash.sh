#!/bin/bash
IMAGE=tbd_prod_stable
DIR_DATA=/mnt/batch/tasks/fsmounts/firestarr_data
CONTAINER=`docker ps | grep ${IMAGE} | sed "s/.* \([^ ]*$\)/\1/g"`
if [ -z "${CONTAINER}" ]; then
  echo Nothing running
  CONTAINER=`docker ps -a | grep ${IMAGE} | head -n 1 | sed "s/.* \([^ ]*$\)/\1/g"`
    if [ -z "${CONTAINER}" ]; then
        echo No existing container so making new one
        USE_IMAGE=registrycwfisdev.azurecr.io/firestarr/${IMAGE}:latest
    else
        echo Restarting based on ${CONTAINER}
        # not running so need to save and start
        USE_IMAGE=${IMAGE}:latest
        docker commit ${CONTAINER} ${USE_IMAGE}
    fi
    docker run -it --entrypoint /bin/bash --workdir /appl/tbd -v ${DIR_DATA}:/appl/data ${USE_IMAGE}
else
  echo Attaching to running container ${CONTAINER}
  docker exec -it ${CONTAINER} /bin/bash
fi

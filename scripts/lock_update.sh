pushd /appl/tbd
/usr/bin/flock -n /tmp/update.lockfile su user /bin/bash -c "./update.sh $*"
popd

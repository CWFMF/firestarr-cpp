((/usr/bin/flock -n /tmp/update.lockfile su user /bin/bash -c '/appl/tbd/update.sh') || rm -rf /tmp/update.lockfile)

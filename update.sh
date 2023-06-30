#!/bin/bash
echo Running update at `date`
source /appl/.venv/bin/activate
cd /appl/tbd
# cmake --configure .
# cmake --build .
# (/usr/bin/flock -n /tmp/update.lockfile -c "(python main.py 14 || echo FAILED)") && /usr/bin/flock -u /tmp/update.lockfile
curl -k "https://spotwx.com/products/grib_index.php?model=geps_0p5_raw&lat=48.80686&lon=-87.45117&tz=-5&label=" | grep "Model date" > /appl/data/geps_latest
# wait until we can get a lock for now
# (diff /appl/data/geps_current /appl/data/geps_latest && echo Model already matches `cat /appl/data/geps_latest) || ((python main.py 14) && (curl -k "https://spotwx.com/products/grib_index.php?model=geps_0p5_raw&lat=48.80686&lon=-87.45117&tz=-5&label=" | grep "Model date") > /appl/data/geps_current)
# /usr/bin/flock -u /tmp/update.lockfile
# do locking outside this script
# (diff /appl/data/geps_current /appl/data/geps_latest && echo Model already matches `cat /appl/data/geps_latest`) || ((python main.py) && (curl -k "https://spotwx.com/products/grib_index.php?model=geps_0p5_raw&lat=48.80686&lon=-87.45117&tz=-5&label=" | grep "Model date") > /appl/data/geps_current)
(diff /appl/data/geps_current /appl/data/geps_latest && echo Model already matches `cat /appl/data/geps_latest`) || ((python main.py) && (cp /appl/data/geps_latest /appl/data/geps_current) && {./publish_geoserver.sh})

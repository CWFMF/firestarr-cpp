#!/bin/bash
set -e
FILES=`grep "was last change before" prev.log | sed "s/.* for src/src/g"`

LAST=`grep "change #" prev.log | sort | tail -n1`
LAST_FILE=`echo ${LAST} | sed "s/.* //g"`


NEXT_REV=`grep "was last change before" prev.log | head -n1 | sed "s/.* was last change before \([^ ]*\) for.*/\1/g"`

echo "Pushing changes from ${NEXT_REV} back to ${LAST}"
# do rebase command
(set -x; $(sed "/git restore --source=.*${LAST_FILE//\//\\\/}/q;" prev.log | tail -n2 | head -n1))

for f in ${FILES}
do
    (set -x; git restore --source=${NEXT_REV} $f)
done

#!/bin/bash
LAST=$(git log --oneline wip -- $1 | head -n1 | sed "s/ .*//g")
(git show -v ${LAST}:$1 \
    || git show -v ${LAST}~1:$1) \
    2>&1 | grep -v fatal | grep -i copyright
echo -e '\n'

# reset && ls -1 src/cpp | xargs -tI {} ./show_latest.sh src/cpp/{}

#!/bin/bash
REV=$(git log --oneline -n1 --grep "$*" | cut -d' ' -f1 -)
if [ "" != "${REV}" ]; then
    ./find_prev.sh ${REV}
else
    echo "No match for \"$*\"" 1>&2
fi

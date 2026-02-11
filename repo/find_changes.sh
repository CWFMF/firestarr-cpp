#!/bin/bash

for r in $(git log --oneline $* | sed "s/ .*//g")
do
    s=$(git log --oneline -n1 --name-status $r)
    # if 2 lines then it's just one file and the revision
    if [ 2 -eq $(echo "${s}" | wc -l) ]; then
        echo "$s" | head -n1
    fi
done


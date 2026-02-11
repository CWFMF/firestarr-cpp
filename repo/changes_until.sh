#!/bin/bash
 for f in $(git log --oneline --name-status -n1 $1 \
    | grep "src/cpp" \
    | sed "s/.*src\/cpp/src\/cpp/g")
do
    if [ 2 -eq $(git log --oneline $1 -- $f | wc -l) ]; then
        # no changes between this commit and other
        # echo $f
        git restore --source=$1 -- $f
    else
        echo $f
        git log --oneline $1 -- $f
    fi
done

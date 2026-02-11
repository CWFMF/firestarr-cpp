#!/bin/bash
./find_prev.sh \
    $(git log --oneline -n1 \
        $(git log --oneline --grep "$1" | cut -d' ' -f1)~1 \
        -- $2 | cut -d' ' -f1) \
    > prev.log

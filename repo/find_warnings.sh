#!/bin/bash
scripts/mk_clean.sh 2>&1 \
    | grep warning \
    | grep '\[' \
    | grep '\]' \
    | sed "s/.*\[\([^]]*\)\]$/\1/g" \
    | sort \
    | uniq

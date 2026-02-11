#!/bin/bash
grep -riE "namespace [a-zA-Z0-9]::" src \
    || ( \
        git rebase --continue \
        && $0
    )

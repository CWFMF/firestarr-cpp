#!/bin/bash
git rebase --continue \
|| ( \
    (git log --oneline -n1 | grep BAD:) \
    && git restore test/output \
    && $0 \
)

#!/bin/bash
$* \
    && git rebase --continue \
    && $0 $*

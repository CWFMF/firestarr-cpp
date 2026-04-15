#!/bin/bash
RUN=$(test/10N_50651.sh RelWithDebInfo $@ -DCPU_TUNE_ONLY=0 -DCPU_TARGET=native -DCMAKE_VERBOSE_OPTIMIZATIONS=1 2>&1 | tee log.txt | grep "Command being timed:" | sed 's/.*Command being timed: "\(.*\)"/\1/')
scripts/profile.sh ${RUN}

#!/bin/bash
dir_out=./cloc
f=${dir_out}/test_$(date -u +"%Y%m%d%H%M").txt
mkdir -p ${dir_out}
git log --oneline -n1 | tee -a ${f}
echo "Outputting to ${f}" | tee ${f}
cloc src/cpp | tee -a ${f}
test/10N_50651.sh 7 2>&1 | tee -a ${f}
git restore test/output

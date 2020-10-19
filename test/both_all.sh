#!/bin/bash
scripts/mk_clean.sh \
  && (test/hourly.sh || echo Hourly failed) \
  && (test/constant_all.sh || echo Constant failed)

#!/bin/bash
scripts/mk_clean.sh \
  && (test/hourly.sh || echo Hourly failed) \
  && (test/constant_one.sh || echo Constant failed)

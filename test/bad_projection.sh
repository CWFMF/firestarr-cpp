#!/bin/bash
rm -rf test/output/bad_projection
./firestarr \
  test/output/bad_projection \
  2023-10-03 \
  58.81228184403946 -122.9117103995713 \
  12:00 \
  --ffmc 93.52 \
  --dmc 18 \
  --dc 409 \
  --apcp_prev 0 \
  --log logfile.txt \
  --no-probability \
  --deterministic \
  -v \
  -i \
  -s \
  --raster-root test/input/bad_projection \
  --perim test/input/bad_projection/perim.tif \
  --wx test/input/bad_projection/weather.csv \
  --output_date_offsets [1,2,3] \
  --tz -7 \
  $@

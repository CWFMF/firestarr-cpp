#!/bin/bash

# testing that grid can load properly at this latitude

rm -rf test/output/north
./firestarr \
  test/output/north \
  2023-06-19 60.82328538629729 -115.70484253475514 19:00 \
  --tz -8 \
  --wx test/input/north/weather.csv \
  --ffmc 83.5 \
  --dmc 53.7 \
  --dc 568.9 \
  --apcp_prev 0 \
  --raster-root test/input/empty_grids \
  --perim test/input/north/perimeter.tif \
  --output_date_offsets [1] \
  --sim-area \
  --deterministic \
  -i \
  $@

#!/bin/bash
# set -x
CMD="/appl/firestarr/firestarr test/output/10N_50651/ 2024-06-03 58.81228184403946 -122.9117103995713 01:00 --dmc 59.5 --dc 450.9 --apcp_prev 0 -v --wx /appl/firestarr/test/input/10N_50651/firestarr_10N_50651_wx.csv --output_date_offsets [1] --tz -5 --raster-root /appl/firestarr/test/input/10N_50651/ --perim /appl/firestarr/test/input/10N_50651/10N_50651.tif"

printf "%12s%12s%12s%12s%12s%12s%12s%12s\n" ffmc confidence salt num_sims min max mean median
# HACK: ffmc doesn't change in wx csv, but diurnal curve calculations on daily values drive probability of spread
for ffmc in {64..100}
do
  for confidence_int in {10..1}
  do
    confidence=$(printf "0.%02d" "${confidence_int}")
    for salt in {0..100}
    do
      output=$($CMD --ffmc $ffmc --confidence $confidence --salt $salt)
      num_sims=$(echo $output | grep -E "Ran .* simulations" | sed "s/.*Ran \(.*\) simulations.*/\1/")
      sizes=$(echo $output | grep -E "Fire size at end of day 1" | sed "s/.*Fire size at end of day 1: \(.*\) ha - \(.*\) ha (mean \(.*\) ha, median \(.*\) ha).*/\1 \2 \3 \4/")
      size_min=$(echo $sizes | cut -d' ' -f1)
      size_max=$(echo $sizes | cut -d' ' -f2)
      size_mean=$(echo $sizes | cut -d' ' -f3)
      size_median=$(echo $sizes | cut -d' ' -f4)
      printf "%12s%12s%12s%12s%12s%12s%12s%12s\n" $ffmc $confidence $salt $num_sims $size_min $size_max $size_mean $size_median
    done
  done
done

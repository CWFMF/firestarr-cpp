#!/bin/bash
echo `date`
curl -k "https://spotwx.com/products/grib_index.php?model=geps_0p5_raw&lat=48.80686&lon=-87.45117&tz=-5&label=" | grep "Model date"

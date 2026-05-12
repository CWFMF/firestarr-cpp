# About FireSTARR

## Overview

FireSTARR is designed to support wildland fire response decision-making, and is the fire growth algorithm originally in the FireGUARD suite of programs.

It focuses on the generation of burn probabilities from replicated simulation of fire growth, smouldering, and natural extinction under the weather and stochastic fire behaviour scenarios.

## Getting started with FireSTARR

`settings.ini`

The `settings.ini` file contains a number of definitions that allow FireSTARR to find the information it requires more rapidly as well as a number of constants for use within your simulation.

Critical components are the fuel look up table (how FireStarr knows what the numbers in your fuel grid represent), the root to your raster data, and the output date offsets (what your forecast horrizons are).


`RASTER_ROOT = ./generated/grid/100m` Root directory to read rasters from.
FireSTARR will read from this location without explicit definition

`MINIMUM_ROS = 0.0`Minimum rate of spread before fire is considered to actually be spreading. If this is above 0 it would yield results where the fire is not moving so long as the rate of spread is below the value defined here.

`MAX_SPREAD_DISTANCE = 0.4` Maximum distance that head can spread per step (* cell size).This setting will ensure very fast fires are polling the underlying data so it's always spreading the fuels represented by the underlying grid. If you go beyond 1 assessments of the fuel grid would not

`OUTPUT_DATE_OFFSETS = [1,2,3,7,14]` Days to output probability contours for. In this case, days 1, 2, 3 7 and 14 days.

`FUEL_LOOKUP_TABLE = ./fuel.lut` Lookup table for fuels (prometheus format)

`MINIMUM_FFMC = 65.0` Minimum ffmc for fire to spread

`MINIMUM_FFMC_AT_NIGHT = 85.0` Minimum ffmc for fire to spread at night

`OFFSET_SUNRISE = 0.0` Time after sunrise to start burning (hours)

`OFFSET_SUNSET = 0.0` Time before sunset to stop burning (hours)

`THRESHOLD_SCENARIO_WEIGHT = 1.0` Weight given to the scenario thresholds

`THRESHOLD_DAILY_WEIGHT = 3.0` Weight given to the daily thresholds

`THRESHOLD_HOURLY_WEIGHT = 2.0` Weight given to the hourly thresholds

`DEFAULT_PERCENT_CONIFER = 50` Default M-1/M-2 percent conifer if none specified

`DEFAULT_PERCENT_DEAD_FIR = 30` Default M-3/M-4 percent dead fir if none specified

`MAXIMUM_TIME = 0` Maximum amount of time to take for simulation (seconds) (0 is unlimited)

`INTERIM_OUTPUT_INTERVAL = 240` Amount of time between generating interim outputs (seconds)

`MAXIMUM_SIMULATIONS = 10000` Maximum number of simulations to do (0 is exactly 1 simulation per scenario)

`CONFIDENCE_LEVEL = 0.1` Maximum percent change in statistics between runs before results are consider stable [0 - 1]

`INTENSITY_MAX_LOW = 2000` Intensity considered to be top of the range (kW/m)

`INTENSITY_MAX_MODERATE = 4000` Intensity considered to be top of the range (kW/m)



## Command Line Interface Usage

### Basic Help Printout

```
Arguments are:
E:\firestarr\firestarr.exe

Usage: firestarr.exe <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [OPTION]...

Run simulations and save output in the specified directory

Usage: firestarr.exe surface <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [OPTION]...

Calculate probability surface and save output in the specified directory

Usage: firestarr.exe test <output_dir> [OPTION]...

Run test cases and save output in the specified directory

 Input Options
   -h                        Show help
   -v                        Increase output level
   -q                        Decrease output level
   --ascii                   Save grids as .asc
   --no-tiff                 Do not save grids as .tif
   -i                        Save individual maps for simulations
   -s                        Run in synchronous mode
   --points                  Save simulation points to file
   --no-intensity            Do not output intensity grids
   --no-probability          Do not output probability grids
   --occurrence              Output occurrence grids
   --sim-area                Output simulation area grids
   --raster-root             Use specified directory as raster root
   --fuel-lut                Use specified fuel lookup table
   --tz                      UTC offset (hours)
   --curing                  Specify static grass curing
   --force-greenup           Force green up for all fires
   --force-no-greenup        Force no green up for all fires
   --log                     Output log file
   --wx                      Input weather file
   --deterministic           Run deterministically (100% chance of spread & survival)
   --confidence              Use specified confidence level
   --perim                   Start from perimeter
   --size                    Start from size
   --ffmc                    Startup Fine Fuel Moisture Code
   --dmc                     Startup Duff Moisture Code
   --dc                      Startup Drought Code
   --apcp_prev               Startup precipitation between 1200 yesterday and start of hourly weather
   --output_date_offsets     Override output date offsets
   ```

#### Extra Detail

`-i                        Save individual maps for simulations`

There is a lot

`--wx                      Input weather file`

This requires the location to and the name of the file with it's extension.

Whole address:

`E:\Data\Burning Information\Weather.csv`

Relative address from current working directory:

`.\Data\Weather.csv`

### Example use of the tool

Note: FireSTARR will look to the location it is invoked from for it's information to run. Any datasets the model requires need to be defined with appropriate flags.

`.\firestarr.exe Output_Fire 2026-05-01 53 -100 13:00 --ffmc 80 --dmc 60 --dc 300 -tz 8`

FFMC, DMC, DC and Timezone are mandatory fields to run FireSTARR

## Additional License Condition

All covered works (e.g., copies of this work, derived works) must include a copy of the file (or an updated version of it) that details credits for work up to the time of the original open source release. That file is available [here](./ORIGIN.md).

## Publications

FireGUARD is published at the following locations:

- [Weather forecast model](https://doi.org/10.3390/fire3020016)
- Burn probability model (In progress)
- [Impact and likelihood-weighted impact model](https://doi.org/10.1071/WF18189)

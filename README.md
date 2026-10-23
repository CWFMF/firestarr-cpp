```{=html}
<?xml version="1.0" encoding="UTF-8"?>
```
```{=html}
<?asciidoc-toc?>
```
```{=html}
<?asciidoc-numbered?>
```
```{=html}
<article xmlns="http://docbook.org/ns/docbook" xmlns:xl="http://www.w3.org/1999/xlink" version="5.0" xml:lang="en">
```
`<info>`{=html}
```{=html}
<title>
```
Project Documentation
```{=html}
</title>
```
`<date>`{=html}2026-06-26`</date>`{=html} `</info>`{=html}
```{=html}
<section xml:id="_overview">
```
```{=html}
<title>
```
Overview
```{=html}
</title>
```
```{=html}
<simpara>
```
FireSTARR is designed to support wildland fire response decision-making,
and is the fire growth algorithm originally in the FireGUARD suite of
programs.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
It focuses on the generation of burn probabilities from replicated
simulation of fire growth, smouldering, and natural extinction under the
weather and stochastic fire behaviour scenarios.
```{=html}
</simpara>
```
```{=html}
</section>
```
```{=html}
<section xml:id="usage">
```
```{=html}
<title>
```
Usage
```{=html}
</title>
```
```{=html}
<simpara>
```
Specifies the arguments for use when using FireSTARR.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
string
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Required
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Yes
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
None
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
<simpara>
```
Example:
```{=html}
</simpara>
```
```{=html}
<screen>
```
firestarr.exe \<output_dir\> \<yyyy-mm-dd\> \<lat\> \<lon\> \<HH:MM\>
\[OPTION\]...
```{=html}
</screen>
```
```{=html}
<section xml:id="_basic_help_printout">
```
```{=html}
<title>
```
Basic Help Printout
```{=html}
</title>
```
```{=html}
<simpara>
```
Arguments are:
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}E:`\firestarr`{=tex}`\firestarr`{=tex}.exe`</literal>`{=html}
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}Usage: firestarr.exe \<output_dir\> \<yyyy-mm-dd\>
\<lat\> \<lon\> \<HH:MM\> \[OPTION\]...​`</literal>`{=html}
```{=html}
</simpara>
```
```{=html}
<programlisting language="Run simulations and save output in the specified directory" linenumbering="unnumbered">
```
Usage: firestarr.exe surface \<output_dir\> \<yyyy-mm-dd\> \<lat\>
\<lon\> \<HH:MM\> \[OPTION\]...

Calculate probability surface and save output in the specified directory

Usage: firestarr.exe test \<output_dir\> \[OPTION\]...

Run test cases and save output in the specified directory

Input Options -h Show help -v Increase output level -q Decrease output
level --ascii Save grids as .asc --no-tiff Do not save grids as .tif -i
Save individual maps for simulations -s Run in synchronous mode --points
Save simulation points to file --no-intensity Do not output intensity
grids --no-probability Do not output probability grids --occurrence
Output occurrence grids --sim-area Output simulation area grids
--raster-root Use specified directory as raster root --fuel-lut Use
specified fuel lookup table --tz UTC offset (hours) --curing Specify
static grass curing --force-greenup Force green up for all fires
--force-no-greenup Force no green up for all fires --log Output log file
--wx Input weather file --deterministic Run deterministically (100%
chance of spread & survival) --confidence Use specified confidence level
--perim Start from perimeter --size Start from size --ffmc Startup Fine
Fuel Moisture Code --dmc Startup Duff Moisture Code --dc Startup Drought
Code --apcp_prev Startup precipitation between 1200 yesterday and start
of hourly weather --output_date_offsets Override output date offsets
```{=html}
</programlisting>
```
```{=html}
</section>
```
```{=html}
<section xml:id="_example_use_of_the_tool">
```
```{=html}
<title>
```
Example use of the tool
```{=html}
</title>
```
```{=html}
<simpara>
```
Note: FireSTARR will look to the location it is invoked from for it's
information to run. Any datasets the model requires need to be defined
with appropriate flags.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}.`\firestarr`{=tex}.exe Output_Fire 2026-05-01 53 -100
13:00 --ffmc 80 --dmc 60 --dc 300 --tz 8`</literal>`{=html}
```{=html}
</simpara>
```
```{=html}
<simpara>
```
FFMC, DMC, DC and Timezone are mandatory fields to run FireSTARR
```{=html}
</simpara>
```
```{=html}
</section>
```
```{=html}
</section>
```
```{=html}
<section xml:id="_settings">
```
```{=html}
<title>
```
Settings
```{=html}
</title>
```
```{=html}
<simpara xml:id="settings.ini">
```
The settings.ini file contains a number of definitions that allow
FireSTARR to find the information it requires more rapidly as well as a
number of constants for use within your simulation.
```{=html}
</simpara>
```
```{=html}
</section>
```
```{=html}
<section xml:id="_critical_components">
```
```{=html}
<title>
```
Critical components
```{=html}
</title>
```
```{=html}
<simpara>
```
The fuel look up table (how FireStarr knows what the numbers in your
fuel grid represent), the root to your raster data, and the output date
offsets (what your forecast horrizons are).
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}RASTER_ROOT`</literal>`{=html} = ./generated/grid/100m
Root directory to read rasters from. FireSTARR will read from this
location without explicit definition
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}MINIMUM_ROS`</literal>`{=html} = 0.0Minimum rate of
spread before fire is considered to actually be spreading. If this is
above 0 it would yield results where the fire is not moving so long as
the rate of spread is below the value defined here.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}MAX_SPREAD_DISTANCE`</literal>`{=html} = 0.4 Maximum
distance that head can spread per step (\* cell size).This setting will
ensure very fast fires are polling the underlying data so it's always
spreading the fuels represented by the underlying grid. If you go beyond
1 assessments of the fuel grid would not
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}OUTPUT_DATE_OFFSETS`</literal>`{=html} =
\[1,2,3,7,14\] Days to output probability contours for. In this case,
days 1, 2, 3 7 and 14 days.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}FUEL_LOOKUP_TABLE`</literal>`{=html} = ./fuel.lut
Lookup table (LUT) for fuels (prometheus format). The Lookup table is
also the location where you would provide meaningful overrides for
non-fuels. The LUT that is shipped with FireSTARR has non-fuels (except
water) being turned into D-2 or some low conifer M series fuel. This
ensure some spread through non-fuel.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}MINIMUM_FFMC`</literal>`{=html} = 65.0 Minimum ffmc
for fire to spread. This is essentially a global burning condition,
without an FFMC over 65, fire will not spread.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}MINIMUM_FFMC_AT_NIGHT`</literal>`{=html} = 85.0
Minimum ffmc for fire to spread at night. This is essentially a global
burning condition, without an FFMC over 85, fire will not spread.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}OFFSET_SUNRISE`</literal>`{=html} = 0.0 Time after
sunrise to start burning (hours). Another global burning condition to
limit fire spread.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}OFFSET_SUNSET`</literal>`{=html} = 0.0 Time before
sunset to stop burning (hours). Another global burning condition to
limit fire spread.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}THRESHOLD_SCENARIO_WEIGHT`</literal>`{=html} = 1.0
Weight given to the scenario thresholds
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}THRESHOLD_DAILY_WEIGHT`</literal>`{=html} = 3.0 Weight
given to the daily thresholds
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}THRESHOLD_HOURLY_WEIGHT`</literal>`{=html} = 2.0
Weight given to the hourly thresholds
```{=html}
</simpara>
```
```{=html}
<simpara>
```
The spread potential thresholds are set using 3 separate random numbers,
scenario, day and hour. These numbers are all drawn independently and
aggregated into a total weight after being multiple against their
weights as established with the THRESHOLD\_\*\*\*\_WEIGHT.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
Spread Example 1
```{=html}
</simpara>
```
```{=html}
<informalfigure>
```
```{=html}
<mediaobject>
```
`<imageobject>`{=html}
`<imagedata fileref="../images/Spread_1.png"/>`{=html}
`</imageobject>`{=html} `<textobject>`{=html}`<phrase>`{=html}Spread
Example 1`</phrase>`{=html}`</textobject>`{=html}
```{=html}
</mediaobject>
```
```{=html}
</informalfigure>
```
```{=html}
<simpara>
```
Spread Example 2
```{=html}
</simpara>
```
```{=html}
<informalfigure>
```
```{=html}
<mediaobject>
```
`<imageobject>`{=html}
`<imagedata fileref="../images/Spread_2.png"/>`{=html}
`</imageobject>`{=html} `<textobject>`{=html}`<phrase>`{=html}Spread
Example 2`</phrase>`{=html}`</textobject>`{=html}
```{=html}
</mediaobject>
```
```{=html}
</informalfigure>
```
```{=html}
<simpara>
```
What this looks like in practice is as follows.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
Wherever the weighted simulation + daily + total threshold is above the
value required for spread then the fire will spread. With the same
scenario for weather, there can still be different outcomes. For
example, if the scenario produced the sine-wave for the rate of spread,
you can see that for the 2nd scenario's 1st day, the rate of spread is
above the threshold most of the time, and thus it would spread, whereas
the 1st scenario would spread briefly before noon, but otherwise not
spread during that day.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}DEFAULT_PERCENT_CONIFER`</literal>`{=html} = 50
Default M-1/M-2 percent conifer if none specified
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}DEFAULT_PERCENT_DEAD_FIR`</literal>`{=html} = 30
Default M-3/M-4 percent dead fir if none specified
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}MAXIMUM_TIME`</literal>`{=html} = 0 Maximum amount of
time to take for simulation (seconds) (0 is unlimited)
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}INTERIM_OUTPUT_INTERVAL`</literal>`{=html} = 240
Amount of time between generating interim outputs (seconds)
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}MAXIMUM_SIMULATIONS`</literal>`{=html} = 10000 Maximum
number of simulations to do (0 is exactly 1 simulation per scenario)
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}CONFIDENCE_LEVEL`</literal>`{=html} = 0.1 Maximum
percent change in statistics between runs before results are consider
stable \[0 - 1\]
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}INTENSITY_MAX_LOW`</literal>`{=html} = 2000 Intensity
considered to be top of the range (kW/m)
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}INTENSITY_MAX_MODERATE`</literal>`{=html} = 4000
Intensity considered to be top of the range (kW/m)
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Setting
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
RASTER_ROOT
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
./generated/grid/100m
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
MINIMUM_ROS
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
0.0
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
MAX_SPREAD_DISTANCE
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
0.4
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
OUTPUT_DATE_OFFSETS
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
\[1,2,3,7,14\]
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
FUEL_LOOKUP_TABLE
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
./fuel.lut
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
MINIMUM_FFMC
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
65.0
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
MINIMUM_FFMC_AT_NIGHT
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
85.0
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
OFFSET_SUNRISE
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
0.0
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
OFFSET_SUNSET
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
0.0
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
THRESHOLD_SCENARIO_WEIGHT
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
1.0
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
THRESHOLD_DAILY_WEIGHT
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
3.0
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
THRESHOLD_HOURLY_WEIGHT
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
2.0
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
DEFAULT_PERCENT_CONIFER
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
50
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
DEFAULT_PERCENT_DEAD_FIR
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
30
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
MAXIMUM_TIME
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
0
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
INTERIM_OUTPUT_INTERVAL
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
240
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
MAXIMUM_SIMULATIONS
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
10000
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
CONFIDENCE_LEVEL
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
0.1
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
INTENSITY_MAX_LOW
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
2000
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
INTENSITY_MAX_MODERATE
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
4000
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="_command_line_arguments">
```
```{=html}
<title>
```
Command Line Arguments
```{=html}
</title>
```
```{=html}
<section xml:id="help">
```
```{=html}
<title>
```
-h
```{=html}
</title>
```
```{=html}
<simpara>
```
Show help information and usage instructions.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Off
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
<simpara>
```
`</simpara> </section> <section xml:id="verbosity-increase"> <title>-v</title> <simpara>Increase output verbosity level. Can typically be repeated for more detail. eg. <literal>-vvv</literal></simpara> <informaltable frame="all" rowsep="1" colsep="1"> <tgroup cols="2"> <colspec colname="col_1" colwidth="25*"/> <colspec colname="col_2" colwidth="75*"/> <tbody> <row> <entry align="left" valign="top"><simpara>Type</simpara></entry> <entry align="left" valign="top"><simpara>Flag</simpara></entry> </row> <row> <entry align="left" valign="top"><simpara>Default</simpara></entry> <entry align="left" valign="top"><simpara>Normal verbosity</simpara></entry> </row> </tbody> </tgroup> </informaltable> </section> <section xml:id="verbosity-decrease"> <title>-q</title> <simpara>Decrease output verbosity level.</simpara> <informaltable frame="all" rowsep="1" colsep="1"> <tgroup cols="2"> <colspec colname="col_1" colwidth="25*"/> <colspec colname="col_2" colwidth="75*"/> <tbody> <row> <entry align="left" valign="top"><simpara>Type</simpara></entry> <entry align="left" valign="top"><simpara>Flag</simpara></entry> </row> <row> <entry align="left" valign="top"><simpara>Default</simpara></entry> <entry align="left" valign="top"><simpara>Normal verbosity</simpara></entry> </row> </tbody> </tgroup> </informaltable> </section> <section xml:id="arg-ascii"> <title>--ascii</title> <simpara>Save output grids in ASCII (<literal>.asc</literal>) format.</simpara> <informaltable frame="all" rowsep="1" colsep="1"> <tgroup cols="2"> <colspec colname="col_1" colwidth="25*"/> <colspec colname="col_2" colwidth="75*"/> <tbody> <row> <entry align="left" valign="top"><simpara>Type</simpara></entry> <entry align="left" valign="top"><simpara>Flag</simpara></entry> </row> <row> <entry align="left" valign="top"><simpara>Default</simpara></entry> <entry align="left" valign="top"><simpara>Disabled</simpara></entry> </row> </tbody> </tgroup> </informaltable> </section> <section xml:id="arg-no-tiff"> <title>--no-tiff</title> <simpara>Disable saving output grids in TIFF (<literal>.tif</literal>) format.</simpara> <informaltable frame="all" rowsep="1" colsep="1"> <tgroup cols="2"> <colspec colname="col_1" colwidth="25*"/> <colspec colname="col_2" colwidth="75*"/> <tbody> <row> <entry align="left" valign="top"><simpara>Type</simpara></entry> <entry align="left" valign="top"><simpara>Flag</simpara></entry> </row> <row> <entry align="left" valign="top"><simpara>Default</simpara></entry> <entry align="left" valign="top"><simpara>TIFF enabled</simpara></entry> </row> </tbody> </tgroup> </informaltable> <simpara>`
```{=html}
</simpara>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-i">
```
```{=html}
<title>
```
-i
```{=html}
</title>
```
```{=html}
<simpara>
```
Save individual simulation maps.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Disabled
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-s">
```
```{=html}
<title>
```
-s
```{=html}
</title>
```
```{=html}
<simpara>
```
Run simulation in synchronous mode.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Asynchronous
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-points">
```
```{=html}
<title>
```
--points
```{=html}
</title>
```
```{=html}
<simpara>
```
Save simulation point outputs to file.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Disabled
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-no-intensity">
```
```{=html}
<title>
```
--no-intensity
```{=html}
</title>
```
```{=html}
<simpara>
```
Disable output of fire intensity grids.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Enabled
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-no-probability">
```
```{=html}
<title>
```
--no-probability
```{=html}
</title>
```
```{=html}
<simpara>
```
Disable output of probability grids.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Enabled
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-occurrence">
```
```{=html}
<title>
```
--occurrence
```{=html}
</title>
```
```{=html}
<simpara>
```
Enable output of fire occurrence grids.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Disabled
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-sim-area">
```
```{=html}
<title>
```
--sim-area
```{=html}
</title>
```
```{=html}
<simpara>
```
Output simulation area grids.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Disabled
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-raster-root">
```
```{=html}
<title>
```
--raster-root
```{=html}
</title>
```
```{=html}
<simpara>
```
Root directory to read rasters from. FireSTARR will read from the
default location without explicit definition.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Path
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Required
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Yes (context dependent)
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
./generated/grid/100m
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-fuel-lut">
```
```{=html}
<title>
```
--fuel-lut
```{=html}
</title>
```
```{=html}
<simpara>
```
Specify fuel lookup table file.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
File path
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-tz">
```
```{=html}
<title>
```
--tz
```{=html}
</title>
```
```{=html}
<simpara>
```
Specify UTC offset (hours).
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Integer
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Units
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Hours
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-curing">
```
```{=html}
<title>
```
--curing
```{=html}
</title>
```
```{=html}
<simpara>
```
Specify static grass curing value.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Numeric
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-force-greenup">
```
```{=html}
<title>
```
--force-greenup
```{=html}
</title>
```
```{=html}
<simpara>
```
Force green-up conditions for all fires.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-force-no-greenup">
```
```{=html}
<title>
```
--force-no-greenup
```{=html}
</title>
```
```{=html}
<simpara>
```
Force no green-up conditions for all fires.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
<simpara>
```
Unresolved directive in index.adoc - include::arguments/log.adoc\[\]
```{=html}
</simpara>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-wx">
```
```{=html}
<title>
```
--wx
```{=html}
</title>
```
```{=html}
<simpara>
```
Specify input weather file. This requires the location to and the name
of the file with it's extension.
```{=html}
</simpara>
```
```{=html}
<simpara>
```
Whole address:
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}E:`\Data`{=tex}`\Burning `{=tex}Information`\Weather`{=tex}.csv`</literal>`{=html}
```{=html}
</simpara>
```
```{=html}
<simpara>
```
Relative address from current working directory:
```{=html}
</simpara>
```
```{=html}
<simpara>
```
`<literal>`{=html}.`\Data`{=tex}`\Weather`{=tex}.csv`</literal>`{=html}
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
File path
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Required
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Yes
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Manadtory Fields
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
`<literal>`{=html}Date, Time, Temperature, Relative Humidity, Wind
Speed, Wind Direction, Precipitation`</literal>`{=html}
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
<simpara>
```
\`\`
```{=html}
</simpara>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-deterministic">
```
```{=html}
<title>
```
--deterministic
```{=html}
</title>
```
```{=html}
<simpara>
```
Run simulation deterministically (100% probability of spread and
survival).
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Flag
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Default
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Stochastic
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-confidence">
```
```{=html}
<title>
```
--confidence
```{=html}
</title>
```
```{=html}
<simpara>
```
Specify confidence level used in outputs.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Numeric
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-perim">
```
```{=html}
<title>
```
--perim
```{=html}
</title>
```
```{=html}
<simpara>
```
Initialize simulation from a fire perimeter.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Input mode
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-size">
```
```{=html}
<title>
```
--size
```{=html}
</title>
```
```{=html}
<simpara>
```
Initialize simulation using initial fire size.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Numeric
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-ffmc">
```
```{=html}
<title>
```
--ffmc
```{=html}
</title>
```
```{=html}
<simpara>
```
Set starting Fine Fuel Moisture Code (FFMC).
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Numeric
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-dmc">
```
```{=html}
<title>
```
--dmc
```{=html}
</title>
```
```{=html}
<simpara>
```
Set starting Duff Moisture Code (DMC).
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Numeric
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-dc">
```
```{=html}
<title>
```
--dc
```{=html}
</title>
```
```{=html}
<simpara>
```
Set starting Drought Code (DC).
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Numeric
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-apcp-prev">
```
```{=html}
<title>
```
--apcp_prev
```{=html}
</title>
```
```{=html}
<simpara>
```
Set precipitation from 1200 previous day to start of hourly weather.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Numeric
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html} `<row>`{=html}
`<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Units
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
mm
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
<section xml:id="arg-output-date-offsets">
```
```{=html}
<title>
```
--output_date_offsets
```{=html}
</title>
```
```{=html}
<simpara>
```
Override default output date offsets.
```{=html}
</simpara>
```
```{=html}
<informaltable frame="all" rowsep="1" colsep="1">
```
`<tgroup cols="2">`{=html}
`<colspec colname="col_1" colwidth="25*"/>`{=html}
`<colspec colname="col_2" colwidth="75*"/>`{=html}
```{=html}
<tbody>
```
`<row>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Type
```{=html}
</simpara>
```
`</entry>`{=html} `<entry align="left" valign="top">`{=html}
```{=html}
<simpara>
```
Configuration
```{=html}
</simpara>
```
`</entry>`{=html} `</row>`{=html}
```{=html}
</tbody>
```
`</tgroup>`{=html}
```{=html}
</informaltable>
```
```{=html}
</section>
```
```{=html}
</section>
```
```{=html}
<section xml:id="license">
```
```{=html}
<title>
```
License & Publications
```{=html}
</title>
```
```{=html}
</section>
```
```{=html}
<section xml:id="_additional_license_condition">
```
```{=html}
<title>
```
Additional License Condition
```{=html}
</title>
```
```{=html}
<simpara>
```
All covered works (e.g., copies of this work, derived works) must
include a copy of the file (or an updated version of it) that details
credits for work up to the time of the original open source release.
That file is available [here](./ORIGIN.md).
```{=html}
</simpara>
```
```{=html}
</section>
```
```{=html}
<section xml:id="_publications">
```
```{=html}
<title>
```
Publications
```{=html}
</title>
```
```{=html}
<simpara>
```
FireGUARD is published at the following locations:
```{=html}
</simpara>
```
```{=html}
<itemizedlist>
```
`<listitem>`{=html}
```{=html}
<simpara>
```
\[Weather forecast
model\](`<link xl:href="https://doi.org/10.3390/fire3020016">`{=html}https://doi.org/10.3390/fire3020016`</link>`{=html})
```{=html}
</simpara>
```
`</listitem>`{=html} `<listitem>`{=html}
```{=html}
<simpara>
```
Burn probability model (In progress)
```{=html}
</simpara>
```
`</listitem>`{=html} `<listitem>`{=html}
```{=html}
<simpara>
```
\[Impact and likelihood-weighted impact
model\](`<link xl:href="https://doi.org/10.1071/WF18189">`{=html}https://doi.org/10.1071/WF18189`</link>`{=html})
```{=html}
</simpara>
```
`</listitem>`{=html}
```{=html}
</itemizedlist>
```
```{=html}
</section>
```
```{=html}
</article>
```

#!/bin/bash
DIR_ROOT="$(dirname $(realpath "$0"))"
DIR="${DIR_ROOT}/10N_50651"
FILE_SETTINGS_BAK=${DIR_ROOT}/settings.bak

set -e

pushd /appl/firestarr
##############
# ./scripts/mk_clean.sh Release

# DIR_BUILD=/appl/firestarr/build
VARIANT=Release
# VARIANT=Test
# VARIANT=Debug
# USE_TIME=
# if [ "Release" == "${VARIANT}" ]; then
  USE_TIME="/usr/bin/time -v"
# fi
# # # VARIANT="$*"
# # # if [ -z "${VARIANT}" ]; then
# #   # VARIANT=Release
#   # VARIANT=Debug
# # # fi
# # echo Set VARIANT=${VARIANT}
# # rm -rf ${DIR_BUILD} \
# #   && /usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=${VARIANT} -S/appl/firestarr -B${DIR_BUILD} -G "Unix Makefiles" \
# #   && /usr/bin/cmake --build ${DIR_BUILD} --config ${VARIANT} --target all -j 50 --
# /usr/bin/cmake --no-warn-unused-cli -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=${VARIANT} -S/appl/firestarr -B${DIR_BUILD} -G "Unix Makefiles" \
#   && /usr/bin/cmake --build ${DIR_BUILD} --config ${VARIANT} --target all -j 50 --
# scripts/mk_verbose.sh

opts=""
# opts="--sim-area"
intensity=""
# intensity="-i"
# intensity="--no-intensity"

DAYS="$1"
if [ "" == "${DAYS}" ]; then
  DAYS=7
else
  if [ ! "${DAYS}" -gt 0 ] || [ ! "${DAYS}" -le 14 ]; then
    echo "Number of days must be an integer between 1 and 14 inclusive but got: ${DAYS}"
    exit
  fi
fi

scripts/mk_clean.sh ${VARIANT}

#################
pushd ${DIR}

dates="[$(seq -s, ${DAYS})]"
dir_out="${dates}/"

rm -rf "${dir_out}"
mkdir -p "${dir_out}"

#   s     s    s    s  #

# Days
#    1    1-3    1-7    1-14
#   s     s    s    s  #


#   1s     1s     9s     83s  # 05ff193ba CHANGE: remove points from cells that can't spread anywhere

#   1s     1s    16s    164s  # d41fa7d06 change Semaphore to use hardware thread limit

# changed from WSL to actual Linux
# lots of screwing around to try making new spread, ended up figuring out old spread didn't
# blank out surrounded/extinguished cells

#   3s     3s    21s    186s  # 77db75f67 move apply_offsets_spreadkey() into calling code
#   3s     3s    21s    179s  # 652cc493b keep calculated offsets after duration since duration is usually the same for each step
#   2s     3s    21s    179s  # 92d859c2f don't pre-generate offsets after duration
#   2s     3s    21s    177s  # a049eb01a merging spread code functions
#   3s     3s    21s    185s  # cc0935feb consolidate code for spreading in Scenario for now
#   2s     3s    21s    181s  # bbdaa8faa removed DEBUG_POINTS code
#   2s     3s    21s    181s  # 6fbc21281 use vector instead of set for offsets_after_duration
#   3s     3s    24s    222s  # 0a12b8e26 make insert() loop vectorizable
#   4s     4s    38s    340s  # 86e28e751 remove insert_() and put code directly in insert()
#   4s     4s    44s    337s  # 50edf27a2 mark source for spread in CellPoints::insert()
#   4s     4s    39s    345s  # 2ce776bbd don't cache unique points since only logging needs them twice
#   3s     4s    40s    343s  # 52a6b0920 use empty functions for LogPoints when not logging
#   3s     4s    37s    340s  # 3df259049 use transform for finding unique points
#   4s     4s    42s    368s  # 154ba9ff9 use tranform to apply offsets
#   4s     5s    55s    432s  # 07368e28f return empty set from unique() if any point is invalid because that implies they all are
#   5s     5s    56s    439s  # 544f1ca92 turn off avx512 since results don't match with it on and it's slower

# REVERT AFTER REARRANGING COMMITS
#   5s     5s    56s    439s  # 544f1ca92 turn off avx512 since results don't match with it on and it's slower

#   s     s    51s    s  # _Float16

# REVERT TO GCC13.3
# BREAKING CHANGE
#   4s     4s    55s    445s  # 44fa98043 disable avhx512 due to performance issues and removing things that are blocking vectorization

# BREAKING CHANGE
#   4s     5s    57s    461s  # 50c93b2cf change ARCH_PREFERRED to 'skylake' from 'skylake-avx512'
#   4s     4s    57s    461s  # 7d7bc53cf try using gcc 14 to get avx10.1-256 to work but getting "cc1plus: warning: Vector size conflicts between AVX10.1 and AVX512, using 512 as max vector size"
#   s     s    s    s  # 44fa98043 disable avhx512 due to performance issues and removing things that are blocking vectorization


# apparently avx512 just sucks:
# https://aloiskraus.wordpress.com/2018/06/16/why-skylakex-cpus-are-sometimes-50-slower-how-intel-has-broken-existing-code/
#   4s     4s    54s    430s  # -march="skylake-avx512" -mno-avx512f

# working on rewriting things so -mavx isn't slower
# NOTE: pm run python script was going during 458s
#   4s     4s    54s    458s  # -mavx
#   s     s    s    s  # -march="native"
#   s     11s    s    s  # -march="skylake-avx512"


#   s     s    55s    s  # no -march
#   11s     12s    37s    656s  # -march="skylake-avx512"
#   12s     11s    37s    683s  # -march="native"
#   s     5s    s    s  # -mtune="skylake-avx512"
#   s     s    s    s  # -mtune="native"


# same thing but set MAXIMUM_TIME to not be 0
#   11s     s    s    s  # c9ecfaadc use -march="skylake-avx512" if supported on machine since azure batch uses that as well
# BREAKING CHANGE
# minor differences in rasters that were checked (probably just rounding difference)
#   11s     11s    38s    715s  # c9ecfaadc use -march="skylake-avx512" if supported on machine since azure batch uses that as well

# multiple changes since 001f6bb5a
#   4s     4s    57s    449s  # 19cd752b1 0.8.15.0

#   s     s    70s    s       # 001f6bb5a only keep final version of each run in azure
#   s     s    s    s         # 73bcfc1ce use at least one thread per scenario
#   4s     4s    70s    534s  # 3d33d3316 cache unique points in CellPoints
# BREAKING CHANGE:
# restart from 7ee8a7ba677a331889a4337ff33eb09420c45f2c
#  5s      5s    77s    584s  # 7ee8a7ba6 (HEAD -> dev-patch) use 'const auto&' instead of just 'const auto' in iterator loops



# change .orig folder to have outputs from 476363af8


# BREAKING CHANGE:
# FIXED version 351cdf8606dc463486b8d45c84e14b88bcbc2d0f dropping non-spreading cells
# rebased change so that fix is now in dev-performance
#  11s    11s    83s   1373s # 476363af8 (HEAD -> dev-performance) change Event to contain SpreadInfo for intensity/ros/raz info



# BREAKING CHANGE: fixed spreading inside perimeter
# somewhere along the way changes broke things and fires aren't spreading enough, so times kind of mean nothing
# ALTHOUGH might be able to compare to this:
# ----------- run old version with '-i' flag working now
#  11s    13s    80s          # 4eae5ceb fix '-i' flag not working properly
#          9s    56s    622s  # b6d4f05f hull on initial insert instead of waiting until all spread is calculated
# -----------
#   8s     8s    48s    975s  # e1ee7591 fix cells not being marked as burnt properly causing spread inside fire area since 489c1ad55e1a0d0918fdd910c4c785d720bccbb2



# BREAKING CHANGE: outputs are different extents; fuels & dem export work properly
#   2s     2s    10s     11s  # fa303859 fixed fuel and dem grids not working because areas aren't divisible by tile size

# lots of work in test_slow.sh


# include InnerPos.* from version cbac29d3
#  132s                       # f0b4ff98 define superclass for Offset and InnerPos and specialize them with template for bounds
# GO BACK TO THIS VERSION THAT MATCHES 2f8c808b
#                             # c492dedb removed CellPoints::empty() and fixed DEBUG_POINTS code

# VERY SIGNIFICANT SIZE DIFFERENCES - much smaller fires
# does it only spread one step or someting? sizes are same for every day
# BREAKING CHANGE: cbac29d3
#   2s     2s    11s     11s  # cbac29d3 define superclass for Offset and InnerPos and specialize them with template for bounds
# THIS GIVES DIFFERENT BUT VERY CLOSE RESULTS
# NEXT STEP IS TO GO BACK TO THIS at dev-performance HEAD and improve
# 120s                        # dc2a1913 wrap extra checks in #ifdef DEBUG_POINTS
# ----- this is the baseline for exporting all intensity rasters
#  12s    13s    80s    889s  # c0fa9992 move code into merge_list() override
# --------- really slow for 1 day, so try old versions and see how they did
# 127s                        # dc2a1913 wrap extra checks in #ifdef DEBUG_POINTS
# ----------- run old version with '-i' flag working now
# --- segfaulted
#                             # 24fbe68a REMOVE LATER - TBB TEST SETTINGS
# back up to dev-performance-bkup-20240615 and rearrange history so '-i' fix is right after b6d4f05f

# Days
#    1    1-3    1-7    1-14
#                             # *** current version ***
#                             # 109a0033 wrap extra checks in #ifdef DEUBG_POINTS
# ----------- run old version with '-i' flag working now
#  11s    13s    80s          # 4eae5ceb fix '-i' flag not working properly
#          9s    56s    622s  # b6d4f05f hull on initial insert instead of waiting until all spread is calculated

# Days
#  1-3    1-7    1-14
#   9s    56s    622s  # b6d4f05f hull on initial insert instead of waiting until all spread is calculated
# -----------
# all kinds of screwing around with test_slow.sh
# -----------
# dbf53353 (HEAD -> dev-performance) merge into merged_map_type and return that instead of iterator for now
# 9d405b39 WIP: trying to get iterator to work
# b59d3211 changed operator* overload into after()
# cc693ca8 moved code into MergeIterator
# 76ace4bf try to make iterator version of merge_lists() because of unusuably large memory allocations
# b37a4b4b added merge_lists() that takes two lists
# b66192b8 specify -> const merged_map_type for lambda functions
# 6773f6de merge nested call to merge_list()
# 24661294 got rid of version of merge_list() that took a merged_map_type& and filled it
# 4eae5ceb fix '-i' flag not working properly
# ------------ fixed intensity maps not saving properly with rebase ------------
# inf?                 # f47777ef merge into merged_map_type and return that instead of iterator for now
# inf?                 # 29706856 moved code into MergeIterator
# inf?                 # d2cf8e09 try to make iterator version of merge_lists() because of unusuably large memory allocations
# inf?                 # 141869c5 added merge_lists() that takes two lists
#  15s                 # bc4ba889 specify -> const merged_map_type for lambda functions
#  15s                 # c15481f2 merge nested call to merge_list()
# ------------ started folding code back into itself here ------------
#  14s                 # 7ab6fe6f got rid of version of merge_list() that took a merged_map_type& and filled it
#  13s                 # 0f1f1d01 return const from merge_list since result is never changed
#  13s                 # cdf15b56 move code into merge_list() override
#  13s                 # 918a36b8 use Location instead of Cell since we don't need attributes until we check if it's unburnable
#  14s                 # ad3f0ca6 return InnerPos directly and don't make tuple for arguments
#  15s                 # 507442d1 added merge_list() override that doesn't take a list and returns one it made
#  14s                 # b7534bc8 merge first merge_list() call into calculate_spread()
#  14s                 # c824e594 merge final_merge_maps() into calculate_spread()
#  14s                 # 0fefd889 change Scenario to just call calculate_spread() instead of merge_list() and final_merge_maps() to do spread
#  15s                 # 0c455e77 removed PointSourceMap in favour of functions acting directly on merged_map_type
#  15s                 # 5c56df70 moved using statements to top of file and replaced K, V, & S with Cell, InnerPos, & CellIndex
#  14s                 # 34f1e820 added using statements so we don't need to qualify things with namespace in code
#  15s                 # 087e10ac remove mutex from PointSourceMap since shouldn't ever be modifying same thing in parallel
#  15s                 # 06794d85 moved last transform into another PointSourceMap constructor
#  15s                 # 8f6500f8 merge apply_spread into a PointSourceMap constructor
#  14s                 # 8ed6a0ad merge apply_offsets into PointSourceMap constructor
#  14s                 # e393c24c change iterator to lookup for_cell later instead of making a pair
#  14s                 # 1ff1e4ac move logging of spread points into cleanup step
#  14s                 # 0d52cf3e flipped order of items in maps_direct
# -------------------------------------------------------
# THIS MADE NO SENSE SO REVERTED
# -------------------------------------------------------
#  15s                 # 12b03de1 changed relativeIndex() to use pair<Idx, Idx>
#  14s                 # 0d52cf3e flipped order of items in maps_direct
# -------------------------------------------------------
# THIS MADE NO SENSE SO REVERTED
# -------------------------------------------------------
#  15s                 # 737aaf78 use ColumnMask for relativeIndex()
#  15s                 # 1ae165ca change relativeIndex() to use HashSize arguments
#  14s                 # 0d52cf3e flipped order of items in maps_direct
# -------------------------------------------------------
# THIS WAS GARBAGE PERFORMANCE SO REVERTED
# -------------------------------------------------------
# inf?                 # 6cf49860 trying to move code out of Scenario
# inf?                 # b39ca0a1 realizing that maybe we don't need to use Cells at all for this part?
# inf?                 # 0ff11e22 remove PointSourceMap and use merged_map_type directly
# 172s                 # e902b297 replace OffsetSet with set<InnerPos>
#  20s   137s          # 8efd4d3d try using std::views::join()
#  18s                 # 5abb4b0a apply source at start instead of in second loop
#  17s                 # a310b38a stop using aliases K, V & S, and replace vector<InnerPos> with OffsetSet
#  17s                 # d704ed66 trying without locks
#  17s                 # b3ca9ce9 again, still trying to improve merge_list
# 153s                 # 68adbb30 still trying to improve merge_list
#  18s   124s          # 795319b1 trying to improve merge_list
#  14s                 # 0d52cf3e flipped order of items in maps_direct
#  14s                 # 0a0c2d74 move code into merge_list()
#  15s                 # 233953cf removed unused aliases and moved merge_all() into constructor
#  15s                 # 242b2559 use a single map in PointSourceMap
#  15s                 # 54ff0f8e merge to_map() into constructor
#  14s    96s          # 0f54ff34 removed unused functions
#  14s    98s          # 031e544f change merge() to do map lookup and then par_unseq call
#  15s                 # 0cf5c3ef removed points_merge_values()
#  15s                 # 4be9ca40 removed sources_merge()
#  15s                 # 04e42842 remove unused functions
#  15s                 # 4d9af105 merging PointSourceMap functions
#  14s                 # 5a7b24bf moved code into merge() and merge_all()
#  15s    99s          # 12d07f0d merge PointsMap class into PointSourceMap
#  15s                 # 34b201f4 merged SourcesMap into PointSourceMap
#  14s                 # b8a31cb3 added prefix to PointsMap and SourcesMap functions so merging classes is easier to follow
#  15s                 # a902cfa5 removed copy constructors
#  15s                 # 7cd84c57 merge class aliases into PointSourceMap
#  15s                 # 3a7c612b convert function into constructor for SourcesMap
#  15s                 # 067a85db renamed member variables so PointsMap and SourcesMap are different
#  15s                 # bfb00ac1 remove public accessors from PointSourceMap member variables
#  15s                 # 549cb08a remove single item merge functions
#  15s     97s         # 50946491 improve erge implemenations
#  13s                 # a69bc542 improve PointsMap::merge()
#  20s                 # 5ee358fc added do_par() and changed to use do_each() for sequential for_each() calls for now
#  20s                 # 592b22b2 added PointsMap(auto& p_o) that just calls merge_values()
#  21s                 # 463ad5dc changed final_merge_maps into a method for PointSourceMap
#  20s                 # 1560eb8a changed other code into PointSourceMap constructors
#  20s                 # d7c8105d changed do_merge_maps into constructor for PointSourceMap
#  20s                 # 94dd42bc define PointSourceMap
#  20s                 # 3b6b455a do_merge_maps again at higher scope before final_merge_maps
#  14s                 # c61f1270 merge and update at a higher scope
#  10s                 # 9cb863a9 add SourcesMap
#         59s          # 6e9f2a3c added MergeMap::for_each()
#         58s          # 96a2be05 use do_each() for hull loop
#         58s          # 5ac4e659 added can_spread() to sync on mutex
#         59s          # 28a92c8b added spreading map
#                      # f3ca224e expect no empty point maps after spread
#   9s    56s    622s  # b6d4f05f hull on initial insert instead of waiting until all spread is calculated
#  10s    61s    686s  # fa2ed9a1 add MergeMap
# -------------------------------------------------------
# THIS WAS GARBAGE PERFORMANCE SO REVERTED
# -------------------------------------------------------
#                      # 34514018 use transform_reduce()
#         inf          # acc4cc29 use std::reduce()
# -------------------------------------------------------
#         64s          # 7cded764 move side-effects for logging & bounds check into transform
#         67s          # c395b2d4 (HEAD -> dev-performance) use std::views::transform() to apply duration to offsets
#  11s    65s    771s  # d1f66191 (HEAD -> dev-performance) add to intermediary map and filter before adding to points_
#                      # e825e70f defined multiplication of Offset by duration
#                      # ef754ae4 trying to change into a chain of stl calls instead of a bunch of distinct loops
#  14s    88s    965s  # 01df7967 use cartesian_product()
#                      # 4dcffe53 try make_cross() for iterating
#                      # 44a6f7d6 added do_each()
#                      # 46b5568d added apply_offsets()
#                      # 5c9c6f9f added for_duration()
#                      # f71be643 remove out of bounds check and just mark outside edge unburnable
#                      # b70efa8c use iterator so we can erase without copying keys
#                      # 972663b0 use vector instead of map
#                      # 94d58ba5 don't make offsets map - just use spread_info_
#                      # de29ada5 remove points_old and just move things that need to spread out of points_
#                      # 529aaad3 combine check for maximum ros with keeping cells that aren't spreading as-is
#                      # b1831954 look up PointSet and OffsetSet for each cell once and keep those
#                      # 30b26766 swap points for cells that aren't spreading first so we don't need to copy
#   15s   94s   1052s  # 3ba7dc87 swap and then copy spread points into points_ instead of erasing from points_ where no points in cell
#   15s   90s   1032s  # b23b9966 remove empty cells from points_ so they don't get used for determining max ros on next step
#   17s  114s   1160s  # b1ffdd44 removed unused counting of how many points are in each cell
# -------------------------------------------------------
# NUMBERS ARE NOT COMPARABLE AFTER THIS BECAUSE ONLY DOING ONE ITERATION NOW
# -------------------------------------------------------
#  91s   304s          add to intermediary map and filter before adding to points_
# 127s                 use cartesian_product()
# 116s                 do_each
# 128s                 added for_duration()
# -------------------------------------------------------
# go back in history to do this and then reapply changes after that
# 125s                 remove out of bounds check and just mark outside edge unburnable
# -------------------------------------------------------
# 131s                 use cartesian_product()
# 119s                 do_each
# 122s                 for_duration
# 127s                 -- try multiplying and keeping offsets --
# 123s                 use iterator so we can erase without copying keys
# 124s                 use vector instead of map
# 141s                 don't make offsets map - just use spread_info_
# 134s                 combine check for maximum ros with keeping cells that aren't spreading as-is
# 126s   301s    944s  remove empty cells from points_ so they don't get used for determining max ros on next step
# 136s   302s          removed unused counting of how many points are in each cell
# -------------------------------------------------------
# >>> dev-performance-key_to_cell_to_pts - not used
# 156s                    WIP: moving towards spread implementation performance improvements
# 143s   302s   1086s     changed Scenario::points_ to be map<topo::SpreadKey, map<topo::Cell, PointSet>>
# <<< dev-performance-key_to_cell_to_pts
# -------------------------------------------------------
# 145s   301s   1063s     merge Offset with InnerPos and change to `using InnerPos = Offset;`
# 144s   302s   1103s     fix out of bounds counter for Scenario not being set to 0 on initialization or reset
# -------------------------------------------------------
#  168             21     number of scenarios that run
#

if [ ! -f ${FILE_SETTINGS_BAK} ]; then
  cp /appl/firestarr/settings.ini ${FILE_SETTINGS_BAK}
fi
# make sure it only runs 21 sims
sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 0/g" /appl/firestarr/settings.ini
sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 0/g" /appl/firestarr/settings.ini

${USE_TIME} \
  /appl/firestarr/firestarr ./${dir_out} 2024-06-03 58.81228184403946 -122.9117103995713 01:00 ${intensity} ${opts} --ffmc 89.9 --dmc 59.5 --dc 450.9 --apcp_prev 0 -v --output_date_offsets ${dates} --wx firestarr_10N_50651_wx.csv --perim 10N_50651.tif
RET=$?
# sed -i "s/MAXIMUM_SIMULATIONS = .*/MAXIMUM_SIMULATIONS = 10000/g" /appl/firestarr/settings.ini
# sed -i "s/MAXIMUM_TIME = .*/MAXIMUM_TIME = 36000/g" /appl/firestarr/settings.ini
cp ${FILE_SETTINGS_BAK} /appl/firestarr/settings.ini
if [ "0" -ne "${RET}" ]; then
  echo "Error during execution"
else
  find ${DIR} -type f -name "interim*" | xargs -I {} rm {}
  # debug mode dumps weather files that we can't compare to release mode
  find ${DIR} -type f -name "*wx_*_out*" | xargs -I {} rm {}
  find ${DIR} -type f -name "*.aux.xml" | xargs -I {} rm {}
  dir_orig="${DIR}.orig"
  if [ -d ${dir_orig} ]; then
    # pushd "${dir_orig}"
    # rm -f "*.xml"
    # popd
    # use echo to not quit if diff fails
    # HACK: use && instead of || because if nothing exists after grep then no differences
    (diff --exclude="*.log" -rq "${dir_orig}" "${DIR}/${dir_out}" | grep -v "^Only in") && (echo "Different results" && (diff -rq "${dir_orig}" "${DIR}/${dir_out}" | grep arrival))
  else
    echo "Outputs did not exist so copying to ${dir_orig}"
    cp -r "${DIR}/${dir_out}" "${dir_orig}"
  fi
  tail -n1 "${DIR}/${dir_out}/firestarr.log"
fi

# popd

# popd

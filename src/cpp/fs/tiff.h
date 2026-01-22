/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TIFF_H
#define FS_TIFF_H
#include <geo_normalize.h>
#include <tiffio.h>
#include <xtiffio.h>
#include "Log.h"
namespace fs
{
#ifndef TIFFTAG_GDAL_NODATA
#define TIFFTAG_GDAL_NODATA 42113
#endif
/**
 * Open file and register GeoTIFF tags so we can read and write properly
 * @param filename Name of file to open
 * @param mode Mode to open file with
 * @return Handle to open TIFF with fields registered
 */
TIFF* GeoTiffOpen(const char* const filename, const char* const mode);
template <class R>
[[nodiscard]] R with_tiff(const string_view filename, function<R(TIFF*, GTIF*)> fct)
{
  logging::debug("Reading file %s", string(filename).c_str());
  // suppress warnings about geotiff tags that aren't found
  TIFFSetWarningHandler(nullptr);
  auto tif = GeoTiffOpen(string(filename).c_str(), "r");
  logging::check_fatal(!tif, "Cannot open file %s as a TIF", string(filename).c_str());
  auto gtif = GTIFNew(tif);
  logging::check_fatal(!gtif, "Cannot open file %s as a GEOTIFF", string(filename).c_str());
  //  try
  //  {
  R result = fct(tif, gtif);
  if (tif)
  {
    XTIFFClose(tif);
  }
  if (gtif)
  {
    GTIFFree(gtif);
  }
  GTIFDeaccessCSV();
  return result;
}
}
#endif

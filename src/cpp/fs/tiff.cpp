/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "tiff.h"
namespace fs
{
TIFF* GeoTiffOpen(const char* const filename, const char* const mode)
{
  TIFF* tif = XTIFFOpen(filename, mode);
  // HACK: avoid warning about const char* cast
  char C_GDALNoDataValue[] = "GDALNoDataValue";
  char C_GeoPixelScale[] = "GeoPixelScale";
  char C_GeoTransformationMatrix[] = "GeoTransformationMatrix";
  char C_GeoTiePoints[] = "GeoTiePoints";
  char C_GeoKeyDirectory[] = "GeoKeyDirectory";
  char C_GeoDoubleParams[] = "GeoDoubleParams";
  char C_GeoASCIIParams[] = "GeoASCIIParams";
  static const TIFFFieldInfo xtiffFieldInfo[] = {
    {TIFFTAG_GDAL_NODATA, -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, false, C_GDALNoDataValue},
    {TIFFTAG_GEOPIXELSCALE, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, C_GeoPixelScale},
    {TIFFTAG_GEOTRANSMATRIX,
     -1,
     -1,
     TIFF_DOUBLE,
     FIELD_CUSTOM,
     true,
     true,
     C_GeoTransformationMatrix},
    {TIFFTAG_GEOTIEPOINTS, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, C_GeoTiePoints},
    {TIFFTAG_GEOKEYDIRECTORY, -1, -1, TIFF_SHORT, FIELD_CUSTOM, true, true, C_GeoKeyDirectory},
    {TIFFTAG_GEODOUBLEPARAMS, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, C_GeoDoubleParams},
    {TIFFTAG_GEOASCIIPARAMS, -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, true, C_GeoASCIIParams}
  };
  TIFFMergeFieldInfo(tif, xtiffFieldInfo, sizeof(xtiffFieldInfo) / sizeof(xtiffFieldInfo[0]));
  return tif;
}
}

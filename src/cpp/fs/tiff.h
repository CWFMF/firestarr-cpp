/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TIFF_H
#define FS_TIFF_H
#include "stdafx.h"
#ifndef TIFFTAG_GDAL_NODATA
#define TIFFTAG_GDAL_NODATA 42113
#endif
typedef struct tiff TIFF;
typedef struct gtiff GTIF;
namespace fs
{
class GeoTiff
{
public:
  ~GeoTiff();
  GeoTiff(const string_view filename);
  const char* filename() const { return filename_.c_str(); }
  TIFF* tiff() { return tiff_; }
  GTIF* gtif() { return gtif_; }

private:
  string filename_{};
  TIFF* tiff_{nullptr};
  GTIF* gtif_{nullptr};
};
/**
 * Open file and register GeoTIFF tags so we can read and write properly
 * @param filename Name of file to open
 * @param mode Mode to open file with
 * @return Handle to open TIFF with fields registered
 */
TIFF* GeoTiffOpen(const char* const filename, const char* const mode);
}
#endif

/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_CONSTANTGRID_H
#define FS_CONSTANTGRID_H
#include "stdafx.h"
#include "Grid.h"
#include "Util.h"
namespace fs
{
template <class T, class V>
class ConstantGrid;
;
[[nodiscard]] ConstantGrid<int, int> readTiffInt(const string_view filename, const Point& point);
/**
 * \brief A GridData<T, V, vector<T>> that cannot change once initialized.
 * \tparam T The initialization value type.
 * \tparam V The initialized value type.
 */
template <class T, class V = T>
class ConstantGrid : public GridData<T, V, vector<T>>
{
public:
  ConstantGrid() = default;
  /**
   * \brief Value for grid at given Location.
   * \param location Location to get value for.
   * \return Value at grid Location.
   */
  [[nodiscard]] constexpr T at(const Location& location) const noexcept override
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(
      location.row() >= this->rows() || location.column() >= this->columns(),
      "Out of bounds (%d, %d)",
      location.row(),
      location.column()
    );
#endif
#ifdef DEBUG_POINTS
    {
      const Location loc{location.row(), location.column()};
      logging::check_equal(loc.column(), location.column(), "column");
      logging::check_equal(loc.row(), location.row(), "row");
      // if we're going to use the hash then we need to make sure it actually matches
      logging::check_equal(loc.hash(), location.hash(), "hash");
    }
#endif
    return this->data.at(location.hash());
  }
  template <class P>
  [[nodiscard]] constexpr T at(const Position<P>& position) const noexcept
  {
    return at(Location{position.hash()});
  }
  /**
   * \brief Throw an error because ConstantGrid can't change values.
   */
  // ! @cond Doxygen_Suppress
  void set(const Location&, const T) override
  // ! @endcond
  {
    throw runtime_error("Cannot change ConstantGrid");
  }
  ~ConstantGrid() override = default;
  ConstantGrid(const ConstantGrid& rhs) noexcept = default;
  ConstantGrid(ConstantGrid&& rhs) noexcept = default;
  ConstantGrid& operator=(const ConstantGrid& rhs) noexcept = default;
  ConstantGrid& operator=(ConstantGrid&& rhs) noexcept = default;
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param nodata_input Value that represents no data for type V
   * \param nodata_value Value that represents no data for type T
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xurcorner Upper right corner X coordinate (m)
   * \param yurcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   * \param data Data to set as grid data
   */
  ConstantGrid(
    const MathSize cell_size,
    const Idx rows,
    const Idx columns,
    const V nodata_input,
    const T nodata_value,
    const MathSize xllcorner,
    const MathSize yllcorner,
    const MathSize xurcorner,
    const MathSize yurcorner,
    const string_view proj4,
    vector<T>&& data
  )
    : GridData<T, V, vector<T>>(
        cell_size,
        rows,
        columns,
        nodata_input,
        nodata_value,
        xllcorner,
        yllcorner,
        xurcorner,
        yurcorner,
        proj4,
        std::move(data)
      )
  { }
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param nodata_input Value that represents no data for type V
   * \param nodata_value Value that represents no data for type T
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   * \param initialization_value Value to initialize entire grid with
   */
  ConstantGrid(
    const MathSize cell_size,
    const Idx rows,
    const Idx columns,
    const V nodata_input,
    const T nodata_value,
    const MathSize xllcorner,
    const MathSize yllcorner,
    const string_view proj4,
    const T& initialization_value
  ) noexcept
    : ConstantGrid(
        cell_size,
        rows,
        columns,
        nodata_input,
        nodata_value,
        xllcorner,
        yllcorner,
        proj4,
        std::move(vector<T>(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS, initialization_value))
      )
  { }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \param convert Function taking int and nodata int value that returns T
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, V> readTiff(
    const string_view filename,
    const Point& point,
    std::function<T(V, V)> convert
  )
  {
    logging::debug("Reading a raster where T = %s, V = %s", typeid(T).name(), typeid(V).name());
    auto grid = readTiffInt(filename, point);
    const auto nodata_input = grid.nodataInput();
    T nodata_value = convert(nodata_input, nodata_input);
    logging::check_fatal(
      convert(nodata_input, nodata_input) != nodata_value,
      "Expected nodata value to be returned from convert()"
    );
    vector<T> values(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS, nodata_value);
    std::transform(grid.data.begin(), grid.data.end(), values.begin(), [&](const int v) {
      return convert(static_cast<V>(v), nodata_input);
    });
    ConstantGrid<T, V> result{
      grid.cellSize(),
      grid.rows(),
      grid.columns(),
      static_cast<V>(grid.nodataInput()),
      nodata_value,
      grid.xllcorner(),
      grid.yllcorner(),
      grid.xurcorner(),
      grid.yurcorner(),
      grid.proj4(),
      std::move(values)
    };
    return result;
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, T> readTiff(const string_view filename, const Point& point)
  {
    return readTiff(filename, point, no_convert<T>);
  }

protected:
  tuple<Idx, Idx, Idx, Idx> dataBounds() const override
  {
    Idx min_row = 0;
    Idx max_row = this->rows();
    Idx min_column = 0;
    Idx max_column = this->columns();
    // // #endif
    return tuple<Idx, Idx, Idx, Idx>{min_column, min_row, max_column, max_row};
  }

private:
  /**
   * \brief Constructor
   * \param grid_info GridBase defining Grid area
   * \param nodata_input Value that represents no data for type V
   * \param nodata_value Value that represents no data for type T
   * \param values Values to initialize grid with
   */
  ConstantGrid(
    const GridBase& grid_info,
    const V nodata_input,
    const T nodata_value,
    vector<T>&& values
  )
    : ConstantGrid(
        grid_info.cellSize(),
        nodata_input,
        nodata_value,
        grid_info.xllcorner(),
        grid_info.yllcorner(),
        grid_info.xurcorner(),
        grid_info.yurcorner(),
        string(grid_info.proj4()),
        std::move(values)
      )
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(
      this->data.size() != static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS, "Invalid grid size"
    );
#endif
  }
};
namespace fuel
{
class FuelType;
using FuelGrid = ConstantGrid<const FuelType*, FuelSize>;
}
class Cell;
using ElevationGrid = ConstantGrid<ElevationSize>;
using CellGrid = ConstantGrid<Cell, Topo>;
}
#endif

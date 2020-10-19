/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_LOG_VALUE_H
#define FS_LOG_VALUE_H
#include "stdafx.h"
#include "Index.h"
namespace fs
{
/**
 * \brief A result of calling log(x) for some value of x, pre-calculated at compile time.
 */
class LogValue : public Index<LogValue>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
};
static constexpr LogValue LOG_0_70{-0.15490195998574316928778374140736};
static constexpr LogValue LOG_0_75{-0.12493873660829995313244988619387};
static constexpr LogValue LOG_0_80{-0.09691001300805641435878331582652};
static constexpr LogValue LOG_0_85{-0.07058107428570726667356900039616};
static constexpr LogValue LOG_0_90{-0.04575749056067512540994419348977};
static constexpr LogValue LOG_1_00{0};
}
#endif

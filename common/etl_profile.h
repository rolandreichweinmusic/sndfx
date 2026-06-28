#pragma once

#include "etl/profiles/cpp23_no_stl.h"

#define ETL_USE_ASSERT_FUNCTION
#define ETL_USE_TYPE_TRAITS_BUILTINS
#undef ETL_COMPILER_GENERIC
#define ETL_NO_INITIALIZER_LIST

// The default, just to make sure
#define ETL_CHRONO_SYSTEM_CLOCK_DURATION etl::chrono::nanoseconds
#define ETL_CHRONO_HIGH_RESOLUTION_CLOCK_DURATION etl::chrono::nanoseconds
#define ETL_CHRONO_STEADY_CLOCK_DURATION etl::chrono::nanoseconds

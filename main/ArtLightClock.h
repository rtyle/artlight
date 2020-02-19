#pragma once

#ifdef DerivedArtTask
    #error DerivedArtTask defined
#else
    #define DerivedArtTask ClockArtTask
#endif

#include "ClockArtTask.h"

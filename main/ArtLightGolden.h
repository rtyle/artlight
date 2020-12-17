#pragma once

#ifdef DerivedArtTask
    #error DerivedArtTask defined
#else
    #define DerivedArtTask GoldenArtTask
#endif

#include "GoldenArtTask.h"

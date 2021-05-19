#pragma once

#ifdef DerivedArtTask
    #error DerivedArtTask defined
#else
    #define DerivedArtTask NixieArtTask
#endif

#include "NixieArtTask.h"

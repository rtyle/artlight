#pragma once

#ifdef DerivedArtTask
    #error DerivedArtTask defined
#else
    #define DerivedArtTask CornholeArtTask
#endif

#include "CornholeArtTask.h"

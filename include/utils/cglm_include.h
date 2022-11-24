#ifndef CGLM_INCLUDE_H
#define CGLM_INCLUDE_H

// cglm has plenty of floating-point promotion errors, so this ignores them

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdouble-promotion"

#include "lib/cglm/cglm.h"

#pragma clang diagnostic pop

#endif

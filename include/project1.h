#pragma once
#include "types.h"
#include "io.h"

#if PROJECT == 1
int kernel();
#endif

void setcursor(int x, int y);
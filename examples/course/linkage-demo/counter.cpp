#include "counter.h"
#ifndef SYSTEMS_DEMO_BAD_ODR
int hits = 0;
#endif
int record_hit() { return ++hits; }

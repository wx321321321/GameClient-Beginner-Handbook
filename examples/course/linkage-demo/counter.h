#ifndef HANDBOOK_LINKAGE_DEMO_COUNTER_H
#define HANDBOOK_LINKAGE_DEMO_COUNTER_H

// Define SYSTEMS_DEMO_BAD_ODR only for the deliberately broken link exercise.
#ifdef SYSTEMS_DEMO_BAD_ODR
int hits = 0; // Wrong: each translation unit now defines the same external object.
#else
extern int hits; // Declare one object whose definition lives in counter.cpp.
#endif

int record_hit();
inline int doubled(int value) { return value * 2; }

#endif

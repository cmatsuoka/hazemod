#ifndef HAZE_UTIL_DEBUG_H_
#define HAZE_UTIL_DEBUG_H_

#ifdef DEBUG
#include <cstdio>
#define Debug(...) do { \
    printf("DEBUG: %s:%d: %s: ", __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__); printf("\n"); \
} while (0)
#else
#define Debug(...)
#endif


#endif  // HAZE_UTIL_DEBUG_H_

#ifdef _MSC_VER
#  define inline __inline
#  define M_PI            3.14159265358979323846
#endif

#ifdef VITESS
#include <stdio.h>
extern FILE *LogFilePtr;
#else
#define LogFilePtr stderr
#endif

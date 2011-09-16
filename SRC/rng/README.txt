Sources are taken from GNU GSL 1.6.

The empty include file gsl/gsl_math.h, and the provided config.h
are needed to satisfy #include statements of GSL sources.
config.h contains necessary settings for the MS Visual C++ compiler.

gsl/gsl_randist.h may be included only if parameter names K are changed
to use lower case k, because someone was so unwise to use K as a 
preprocessor constant in general.h.

Files types.c and gsl/gsl_rng.h have been edited to exclude
random number generators with #ifdef SELECT lines.

All references to stderr have been changed to LogFilePtr, because
VITESS modules must write text results pipe-conform to a log file.
To find occurences of stdout a "grep -w stderr *.c" was sufficient.

To use a specific random number generator you have to set the environment
variable GSL_RNG_TYPE with possible values
taus gfsr4 mt19937 ranlux ran3

To set a specific seed value you have to set the env. variable GSL_RNG_SEED.

To compile an object library with Linux or Darwin (in the past Solaris, Tru64) use

gmake libgslran

Then create a subdirectory according to the architecture, as
Linux
  (32 bit)
Linux_x86_64
Darwin_x86_64
OSF1
Solaris

and mv libgslran.a to that subdirectory.



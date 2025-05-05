# gcc -I. default.c gsltest.c types.c -o gsltest

AUXI.o = default.o types.o rng.o error.o stream.o
RNG_O.o = gfsr4.o mt.o ran3.o taus.o ranlux.o
RNG_L.o = gauss.o sphere.o

AUXI.c = $(AUXI.o:.o=.c)
RNG_O.c = $(RNG_O.o:.o=.c)
INCL = -I.

COPTFLAGS = $(CFLAGS) $(INCL) -O3 -fomit-frame-pointer -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CDEBFLAGS = $(CFLAGS) $(INCL) -Wall

SYS = $(shell uname)

ifndef PREFIX
    PREFIX = /usr/local
endif
INCLUDEDIR = $(PREFIX)/include/gsl
LIBDIR = $(PREFIX)/lib
BINDIR = $(PREFIX)/bin

ifeq ($(SYS),Linux)
 UVERS=1
else
 ifeq ($(SYS),Darwin)
  UVERS=1
 endif
endif

ifeq ($(UVERS),1)
 CC = gcc
 ARCH = $(shell uname -m)
 ifeq ($(ARCH),x86_64)
  SUBSYS = "$(SYS)_$(ARCH)"
 else
  SUBSYS = $(SYS)
 endif
else
 CC = /net/bin/gcc
 SUBSYS = $(SYS)
endif

AR           = ar
ARFLAGS      = -rs

all: $(SUBSYS)/libgslran.a

$(SUBSYS)/libgslran.a : $(AUXI.c) $(RNG_O.c)
	$(MAKE) clean
	$(MAKE) libgslran.a
	-mkdir $(SUBSYS)/
	mv libgslran.a $(SUBSYS)/
	-rm *.o

gsltest : gsltest.c $(AUXI.c) $(RNG_O.c)
	$(CC) $(COPTFLAGS) $(INCL) -o $@ $^
	strip gsltest

gsldeb : gsltest.c $(AUXI.c) $(RNG_O.c)
	$(CC) $(CDEBFLAGS) $(INCL) -o $@ $^

gsltestlib : gsltest.c libgslran.a
	$(CC) -DVITESS $(COPTFLAGS) $(INCL) -L. -o $@ $^ -lgslran
	strip gsltestlib

libgslran.a : $(AUXI.o) $(RNG_O.o) $(RNG_L.o)
	$(AR) $(ARFLAGS) $@ $^

all: gsltestlib gsltest

.c.o:
	$(CC) -DVITESS -c $(COPTFLAGS) -o $@ $<

clean :
	-rm *.o
	-rm *.a

install: libgslran.a
	install -d $(LIBDIR) $(INCLUDEDIR)
	install libgslran.a $(LIBDIR)
	install gsl/gsl_errno.h  gsl/gsl_math.h  gsl/gsl_message.h  gsl/gsl_randist.h  gsl/gsl_rng.h  gsl/gsl_types.h $(INCLUDEDIR)

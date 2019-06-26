# gcc -I. default.c gsltest.c types.c -o gsltest

AUXI.o = default.o types.o rng.o error.o stream.o
RNG_O.o = gfsr4.o mt.o ran3.o taus.o ranlux.o
RNG_L.o = gauss.o sphere.o

AUXI.c = $(AUXI.o:.o=.c)
RNG_O.c = $(RNG_O.o:.o=.c)
INCL = -I.

COPTFLAGS = $(INCL) -O3 -fomit-frame-pointer -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CDEBFLAGS = $(INCL) -Wall

SYS = $(shell uname)
SUBSYS = $(SYS)

ifeq ($(SYS),SunOS)
CC = /net/bin/gcc
endif

ifeq ($(SYS),OSF1)
CC = /net/bin/gcc
endif

ifeq ($(SYS),Linux)
CC = gcc
ARCH = (shell uname -i)
ifeq ($(ARCH),x86_64)
SUBSYS = $(SYS)_$(ARCH)
endif
endif

CC = gcc
AR           = ar
ARFLAGS      = -rs

libgslran: $(SUBSYS)/libgslran.a

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


# Unix Makefile for VITESS

SHELL = /bin/sh

ifndef VITESS_VERSION
VITESS_VERSION = 3.6
endif
MODULES_DIR = ../MODULES
GSLPATH = ./rng
G2PATH = ./g2-0.72

SYS := $(shell uname -s)
ARCH := $(shell uname -m)
TODAY := $(shell date '+%Y-%m-%d')
FULLVERSION = $(VITESS_VERSION) $(SYS)_$(ARCH) $(TODAY)

CC = cc
ARCHFLAGS =
CFLAGS = $(ARCHFLAGS) -O3 -pthread -Wall -Wno-cast-qual -fomit-frame-pointer -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I"$(GSLPATH)"
CXX = c++
CXXFLAGS = $(CFLAGS)
CPPFLAGS =
LDFLAGS = -L. -L"$(GSLPATH)"
LDLIBS = -lvitess -lgslran -lm
AR = ar
ARFLAGS = rsv
INSTALL = install
RM = rm -f
LN = ln -s

DARWIN_UNIVERSAL = NO
ifeq ($(SYS),Darwin)
ifeq ($(ARCH),arm64)
DARWIN_UNIVERSAL = YES
ARCHFLAGS = -arch x86_64 -arch arm64
endif
endif

ifndef NOG2
CFLAGS += -DDO_PNG -DDO_X11 -DDO_GD -DVT_GRAPH -I"$(G2PATH)/include"
LDFLAGS += -L"$(G2PATH)"
LDLIBS += -lg2 -lgd -lpng -lz -lfreetype -lXpm -lX11
G2OPTS =
ifeq ($(SYS),Darwin)
G2OPTS += --with-x --x-includes=/opt/X11/include --x-libraries=/opt/X11/lib
CFLAGS += -I/opt/X11/include
LDFLAGS += -L/opt/X11/lib
endif
endif

ifdef DEBUG
CFLAGS := $(filter-out -O3,$(CFLAGS))
CFLAGS += -g -O0 -DDEBUG=1
endif

# Vitess modules to build
ALL = ascii2bin \
beamstop \
bender \
capture_flux \
cas_v40 \
chop_phases \
chopper_disc \
chopper_fermi \
chopper_fermi_parallel \
collimator \
collimator_radial \
collimator_soller \
define_direction \
detector \
direct_view \
eval_elast \
eval_elast2 \
eval_inelast \
eval_sans \
filter \
filter2D \
flipper_coil \
flipper_gradient \
fom \
frame \
gener_batch \
gener_bispectral \
gener_hkl \
gener_pipe \
grid \
guide \
guide_elliptic \
guide_parallel \
guide_shape \
lattice_dist \
lenses \
merge_spectra \
mirror_coating \
mirror_elliptical \
mon1_brl \
mon1_pol \
mon2_div \
mon2_kdiv \
mon2_pos \
mon2_posdiv \
mon2_rdiv \
mon2_tofwl \
mon2_wldiv \
mon_brilliance \
monitor1 \
monitor1D \
monitor2D \
monitorpol_1d \
monitorpol_pos \
monochr_analyser \
monochromator \
opt_sim \
pol_mirror \
polariser_he3 \
polariser_sm \
precessionfield \
read_in \
resonator_drabkin \
rotating_field \
runtime \
sample_elasticisotr \
sample_environment \
sample_inelast \
sample_nxs \
sample_powder \
sample_reflectom \
sample_s_q \
sample_sans \
sample_singcryst \
screen \
sesans_field \
slit \
sm_ensemble \
sm_ensemble_parallel \
sortiap \
source \
space \
spacewindow \
spacewindow_multiple \
spin_reset \
standard_deviation \
surface_file \
velselect \
window \
writeout
ifndef NOG2
ALL += visual
endif

# objects used by multiple modules
COMMON = init.o \
general.o \
convert.o \
message.o \
softabort.o \
intersection.o \
matrix.o \
mathfunctions.o \
sample.o \
mathvector.o \
mathmatrix.o \
mcpl.o \
mon2_header.o \
trace.o \
threadHelper.o \
sswread.o
ifndef NOG2
COMMON += cpgplot.o
endif

# general make targets
.PHONY: all
all: $(ALL)

.PHONY: clean
clean:
	$(RM) *.o *.a $(ALL)
	$(MAKE) -C "$(GSLPATH)" clean
	[ -f "$(G2PATH)/Makefile" ] && $(MAKE) -C "$(G2PATH)" clean

.PHONY: install
ifeq ($(DARWIN_UNIVERSAL),YES)
install: $(ALL)
	for F in $(ALL); do \
		$(INSTALL) $${F} $(MODULES_DIR)/$${F}_$(SYS)_arm64; \
		$(LN) $${F}_$(SYS)_arm64 $(MODULES_DIR)/$${F}_$(SYS)_x86_64; \
	done
else
install: $(ALL)
	for F in $(ALL); do \
		$(INSTALL) $${F} $(MODULES_DIR)/$${F}_$(SYS)_$(ARCH); \
	done
endif


# build libraries
$(G2PATH)/libg2.a:
	cd $(G2PATH); ./configure $(G2OPTS) CC=cc CXX=c++ CFLAGS="$(ARCHFLAGS)" CXXFLAGS="$(ARCHFLAGS)"
	$(MAKE) -C "$(G2PATH)" all

$(GSLPATH)/libgslran.a:
	$(MAKE) -C "$(GSLPATH)" libgslran.a CC=cc CXX=c++ CFLAGS="$(ARCHFLAGS)" CXXFLAGS="$(ARCHFLAGS)"

libvitess.a: $(COMMON)
	$(AR) $(ARFLAGS) $@ $^

ifndef NOG2
cpgplot.o: | $(G2PATH)/libg2.a
endif

$(ALL): | libvitess.a $(GSLPATH)/libgslran.a


# additional dependencies
init.o: CPPFLAGS+=-DVVERS='"$(FULLVERSION)"'

bender: bender.c bendtest.c bendchtr.c bendertr.c bender_inter_data.c

chopper_disc: chopper_disc.c bender_inter_data.c

gener_batch: gener_batch.c gener_fct.c

gener_pipe: gener_pipe.c pipe_fct.c

grid: grid.c bender_inter_data.c

lenses: lenses.c lensetr.c

mirror_elliptical: mirror_elliptical.c mirrrefl.c

monitor1D: monitor1D.cpp mon1D.cpp

monitor2D: monitor2D.cpp mon2D.cpp

monochr_analyser: monochr_analyser.c ma_functions.c ma_geom.c

monochromator: monochromator.cpp monochrclass.cpp

opt_sim: opt_sim.c opt_grad.c opt_grad_mc.c opt_metro.c opt_swarm.c opt_fct.c calc_sim_fom.c

read_in: read_in.c random_sampler.c

sample_nxs: sample_nxs.c nxs.c sgclib.c sgfind.c sghkl.c sgio.c sgsi.c

sample_reflectom: sample_reflectom.c bender_inter_data.c

sample_s_q: sample_s_q.c sq_calc.c

source: source.c src_modchar.c source_csns.c source_ess.c

window: window.c bender_inter_data.c

spacewindow_multiple: spacewindow_multiple.c bender_inter_data.c


# keep old module names for compatibility
chopper_fermi_parallel: chopper_fermi
	cp $^ $@

guide_parallel: guide
	cp $^ $@

mon_brilliance: mon1_brl
	cp $^ $@

monitorpol_1d: mon1_pol
	cp $^ $@

sm_ensemble_parallel: sm_ensemble
	cp $^ $@

spacewindow: window
	cp $^ $@


# suffix rules
.SUFFIXES: .c .cpp .o .exe

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $^

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $^

.c:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.cpp:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.o:
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)


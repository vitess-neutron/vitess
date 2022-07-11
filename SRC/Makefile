# Unix Makefile for VITESS

SHELL = /bin/sh

ifndef VITESS_VERSION
VITESS_VERSION = 3.5
endif
MODULES_DIR = ../MODULES
GSLPATH = ./rng

SYS := $(shell uname -s)
ARCH := $(shell uname -m)
TODAY := $(shell date '+%Y-%m-%d')
FULLVERSION = $(VITESS_VERSION) $(SYS)_$(ARCH) $(TODAY)

CC = cc
CFLAGS = -O3 -pthread -Wall -Wpointer-arith -Wcast-qual -Wwrite-strings -fomit-frame-pointer -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I"$(GSLPATH)"
CXX = c++
CXXFLAGS = $(CFLAGS)
CPPFLAGS=
LDFLAGS = -L. -L"$(GSLPATH)"
LDLIBS = -lvitess -lgslran -lm
AR = ar
ARFLAGS = rv
INSTALL = install
RM = rm -f


ALL = ascii2bin \
beamstop \
bender \
capture_flux \
cas_v40 \
chop_phases \
chopper_disc \
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
flipper_coil \
flipper_gradient \
fom \
frame \
gener_batch \
gener_bispectral \
gener_pipe \
grid \
guide_elliptic \
guide_parallel \
guide_shape \
lattice_dist \
lenses \
merge_spectra \
mirror_coating \
mirror_elliptical \
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
sesans_field \
slit \
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
writeout

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
bender_inter_data.o


.PHONY: all
all: $(ALL)

.PHONY: clean
clean:
	$(RM) *.o *.a $(ALL)
	$(MAKE) -C "$(GSLPATH)" clean

.PHONY: install
install: all
	for F in $(ALL); do \
		$(INSTALL) $${F} $(MODULES_DIR)/$${F}_$(SYS)_$(ARCH); \
	done


"$(GSLPATH)/libgslran.a":
	$(MAKE) -C "$(GSLPATH)" libgslran.a

libvitess.a: $(COMMON)
	$(AR) $(ARFLAGS) $@ $^


$(ALL): | libvitess.a "$(GSLPATH)/libgslran.a"

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

sample_nxs: sample_nxs.c nxs.c sgclib.c sgfind.c sghkl.c sgio.c sgsi.c

sample_s_q: sample_s_q.c sq_calc.c

source: source.c src_modchar.c source_csns.c source_ess.c

spacewindow: spacewindow.c bender_inter_data.c

spacewindow_multiple: spacewindow_multiple.c bender_inter_data.c


.SUFFIXES: .c .cpp .o

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


# Unix Makefile for VITESS

VITESS_VERSION ?= 3.8

MODULES_DIR ?= ../MODULES
GSLPATH ?= ./rng
G2PATH ?= ./g2-0.72
NCRYSTALPATH ?= ./ncrystal
KDSOURCEPATH ?= ./libkdsource

SYS := $(shell uname -s)
ARCH := $(shell uname -m)
TODAY := $(shell date '+%Y-%m-%d')
FULLVERSION := $(VITESS_VERSION) $(SYS)_$(ARCH) $(TODAY)

CC ?= cc
CXX ?= c++
ARCHFLAGS =
CCFLAGS = $(ARCHFLAGS) -pthread -Wall -Wextra -Wno-deprecated-declarations -Wno-sign-compare
CFLAGS = -std=gnu99 $(CCFLAGS)
CXXFLAGS = -std=c++17 $(CCFLAGS)
CPPFLAGS = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I"$(GSLPATH)"
LDFLAGS = -L. -L"$(GSLPATH)"
LDLIBS = -lvitess -lgslran -lm
AR = ar
ARFLAGS = rsv
INSTALL = install -s
RM = rm -f
RMDIR = rm -rf
LN = ln -s
CMAKEFLAGS = -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX)

DARWIN_UNIVERSAL ?= 0
ifeq ($(SYS),Darwin)
NOGD ?= 1
ifeq ($(ARCH),arm64)
ARCHFLAGS = -arch x86_64 -arch arm64
CMAKEFLAGS += "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
endif
else
NOGD ?= 0
endif

ifndef NOG2
CPPFLAGS += -I"$(G2PATH)/include" -DDO_PNG -DDO_X11 -DVT_GRAPH
LDFLAGS += -L"$(G2PATH)"
LDLIBS += -lg2 -lpng -lz -lfreetype -lXpm -lX11
ifeq ($(NOGD), 0)
CPPFLAGS += -DDO_GD
LDLIBS += -lgd
endif
G2OPTS =
ifeq ($(SYS),Darwin)
G2OPTS += --with-x --x-includes=/opt/X11/include --x-libraries=/opt/X11/lib --disable-shared
CPPFLAGS += -I/opt/X11/include
LDFLAGS += -L/opt/X11/lib
endif
endif

DEBUG ?= 0
ifeq ($(DEBUG), 0)
CCFLAGS += -O3
CPPFLAGS += -DNDEBUG
CMAKEFLAGS += -DCMAKE_BUILD_TYPE=Release
ifdef LTO
CCFLAGS += -flto
LDFLAGS += -O3 -flto
endif
else
CCFLAGS += -g -O0 -ffp-contract=off
CPPFLAGS += -DDEBUG=1
CMAKEFLAGS += -DCMAKE_BUILD_TYPE=Debug
INSTALL = install
ifeq ($(DEBUG), 2)
CCFLAGS += -fsanitize=address,undefined
CPPFLAGS += -D_GLIBCXX_ASSERTIONS -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
LDFLAGS += -fsanitize=address,undefined
endif
endif

ifdef AVX
CCFLAGS += -mavx
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
kdsource \
lattice_dist \
lenses \
merge \
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
prism  \
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
template \
velselect \
window \
writeout

ifdef LIBTORCHPATH
ALL += source_ai
endif
ifndef NOG2
ALL += visual dist_time
endif
ifndef NONCRYSTAL
ALL += sample_ncrystal
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
	-$(RM) *.o *.a $(ALL)

.PHONY: distclean
distclean: clean
	$(MAKE) -C "$(GSLPATH)" clean
	-[ -f "$(G2PATH)/Makefile" ] && $(MAKE) -C "$(G2PATH)" clean
	-$(RMDIR) "$(KDSOURCEPATH)/build" "$(KDSOURCEPATH)/install"
	-$(RMDIR) "$(NCRYSTALPATH)/build" "$(NCRYSTALPATH)/install"

.PHONY: install
ifeq ($(DARWIN_UNIVERSAL),1)
install: $(ALL)
	for F in $(ALL); do \
		$(INSTALL) $${F} $(MODULES_DIR)/$${F}_$(SYS)_arm64; \
		$(LN) $${F}_$(SYS)_arm64 $(MODULES_DIR)/$${F}_$(SYS)_x86_64 || true; \
	done
else
install: $(ALL)
	for F in $(ALL); do \
		$(INSTALL) $${F} $(MODULES_DIR)/$${F}_$(SYS)_$(ARCH); \
	done
endif


# build libraries
$(G2PATH)/libg2.a:
	cd $(G2PATH); ./configure $(G2OPTS) --disable-shared CC=$(CC) CXX=$(CXX) CFLAGS="-std=gnu99 $(ARCHFLAGS)" CXXFLAGS="$(ARCHFLAGS)"
	$(MAKE) -C "$(G2PATH)" all

$(GSLPATH)/libgslran.a:
	$(MAKE) -C "$(GSLPATH)" libgslran.a CC=$(CC) CXX=$(CXX) CFLAGS="$(ARCHFLAGS)" CXXFLAGS="$(ARCHFLAGS)"

$(KDSOURCEPATH)/install/lib/libkdsource.a:
	cmake -S"$(KDSOURCEPATH)" -B"$(KDSOURCEPATH)/build" -DCMAKE_INSTALL_PREFIX="$(realpath $(KDSOURCEPATH))/install" -DCMAKE_INSTALL_LIBDIR=lib -DBUILD_SHARED_LIBS=OFF $(CMAKEFLAGS)
	cmake --build $(KDSOURCEPATH)/build $(filter -j%,$(MAKEOPTS))
	cmake --install $(KDSOURCEPATH)/build

$(NCRYSTALPATH)/install/lib/libNCrystal.a:
	cmake -S"$(NCRYSTALPATH)" -B"$(NCRYSTALPATH)/build" -DCMAKE_INSTALL_PREFIX="$(realpath $(NCRYSTALPATH))/install" -DCMAKE_INSTALL_LIBDIR=lib -DBUILD_SHARED_LIBS=OFF $(CMAKEFLAGS)
	cmake --build $(NCRYSTALPATH)/build $(filter -j%,$(MAKEOPTS))
	cmake --install $(NCRYSTALPATH)/build

libvitess.a: $(COMMON)
	$(AR) $(ARFLAGS) $@ $^

ifndef NOG2
cpgplot.o: $(G2PATH)/libg2.a
endif

ifndef NONCRYSTAL
sample_ncrystal: $(NCRYSTALPATH)/install/lib/libNCrystal.a
endif

$(ALL): libvitess.a $(GSLPATH)/libgslran.a


# additional dependencies
init.o: CPPFLAGS+=-DVVERS='"$(FULLVERSION)"'

bender: bender.c bendtest.c bender_path.c bender_inter_data.c

chopper_disc: chopper_disc.c bender_inter_data.c

gener_batch: gener_batch.c gener_fct.c

gener_pipe: gener_pipe.c pipe_fct.c

grid: grid.c bender_inter_data.c

kdsource: CPPFLAGS += -I$(KDSOURCEPATH)/install/include
kdsource: LDFLAGS += -L$(KDSOURCEPATH)/install/lib
kdsource: LDLIBS += -lkdsource -lmcpl -lxml2 -lz
kdsource: kdsource.c | $(KDSOURCEPATH)/install/lib/libkdsource.a

lenses: lenses.c lensetr.c

mirror_elliptical: mirror_elliptical.c mirrrefl.c

monitor1D: monitor1D.cpp mon1D.cpp

monitor2D: monitor2D.cpp mon2D.cpp

monochr_analyser: monochr_analyser.c ma_functions.c ma_geom.c

monochromator: monochromator.cpp monochrclass.cpp

opt_sim: opt_sim.c opt_grad.c opt_grad_mc.c opt_metro.c opt_swarm.c opt_fct.c calc_sim_fom.c

read_in: read_in.c random_sampler.c

sample_ncrystal: CPPFLAGS += -I$(NCRYSTALPATH)/install/include
sample_ncrystal: LDFLAGS += -L$(NCRYSTALPATH)/install/lib
sample_ncrystal: LDLIBS += -lNCrystal -ldl

sample_nxs: sample_nxs.c nxs.c sgclib.c sgfind.c sghkl.c sgio.c sgsi.c

sample_reflectom: sample_reflectom.c bender_inter_data.c

sample_s_q: sample_s_q.c sq_calc.c

source: source.c src_modchar.c source_csns.c source_ess.c

window: window.c bender_inter_data.c

spacewindow_multiple: spacewindow_multiple.c bender_inter_data.c

ifdef LIBTORCHPATH
ifeq ($(DARWIN_UNIVERSAL),1)
# universal build: assume LIBTORCHPATH has subdiretories for architectures
source_ai_arm64: ARCHFLAGS = -arch arm64
source_ai_arm64: CXXFLAGS += -I$(LIBTORCHPATH)/arm64/include -I$(LIBTORCHPATH)/arm64/include/torch/csrc/api/include
source_ai_arm64: LDFLAGS += -L$(LIBTORCHPATH)/arm64/lib
source_ai_arm64: LDLIBS += -ltorch -ltorch_cpu -lc10 -Wl,-rpath,$(LIBTORCHPATH)/arm64/lib
source_ai_arm64: source_ai.cpp | libvitess.a $(GSLPATH)/libgslran.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $(filter-out %.a,$^) $(LDLIBS)

source_ai_x86_64: ARCHFLAGS = -arch x86_64
source_ai_x86_64: CXXFLAGS += -I$(LIBTORCHPATH)/x86_64/include -I$(LIBTORCHPATH)/x86_64/include/torch/csrc/api/include
source_ai_x86_64: LDFLAGS += -L$(LIBTORCHPATH)/x86_64/lib
source_ai_x86_64: LDLIBS += -ltorch -ltorch_cpu -lc10 -Wl,-rpath,$(LIBTORCHPATH)/x86_64/lib
source_ai_x86_64: source_ai.cpp | libvitess.a $(GSLPATH)/libgslran.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $(filter-out %.a,$^) $(LDLIBS)

source_ai: source_ai_arm64 source_ai_x86_64
	lipo -create -output  $@ $(filter-out %.a,$^)
else
source_ai: CXXFLAGS += -I$(LIBTORCHPATH)/include -I$(LIBTORCHPATH)/include/torch/csrc/api/include
source_ai: LDFLAGS += -L$(LIBTORCHPATH)/lib
source_ai: LDLIBS += -ltorch -ltorch_cpu -lc10 -Wl,-rpath,$(LIBTORCHPATH)/lib
source_ai: source_ai.cpp
endif
endif

# keep old module names for compatibility
chopper_fermi_parallel: chopper_fermi
	cp $(filter-out %.a,$^) $@

guide_parallel: guide
	cp $(filter-out %.a,$^) $@

mon_brilliance: mon1_brl
	cp $(filter-out %.a,$^) $@

monitorpol_1d: mon1_pol
	cp $(filter-out %.a,$^) $@

sm_ensemble_parallel: sm_ensemble
	cp $(filter-out %.a,$^) $@

spacewindow: window
	cp $(filter-out %.a,$^) $@


# suffix rules
.SUFFIXES: .c .cpp .o .exe

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $^

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $^

%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(filter-out %.a,$^) $(LDLIBS)

%: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $(filter-out %.a,$^) $(LDLIBS)

%: %.o
	$(CXX) $(LDFLAGS) -o $@ $(filter-out %.a,$^) $(LDLIBS)

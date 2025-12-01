# Vitess NMAKE File
# to be used as
# nmake /f vitess.mak all
# nmake /f vitess.mak install
# after cd to the SRC directory of the Vitess tree
# from a Microsoft VS 2022 cmd.exe

!if "$(VSCMD_ARG_TGT_ARCH)" == "x86"
WINARCH=win32
!else
WINARCH=win64
!endif

VERSION_MAJOR = 3
VERSION_MINOR = 8
GSLPATH = .\rng
G2PATH = .\g2-0.72
NCRYSTALPATH = .\ncrystal\$(WINARCH)
KDSOURCEPATH = .\libkdsource\$(WINARCH)

CC = cl.exe
CXX = cl.exe
CCFLAGS = /MT /nologo /W3 /GF /EHsc /wd5105
CFLAGS = /std:c11 $(CCFLAGS)
CXXFLAGS = /std:c++17 $(CCFLAGS)
CPPFLAGS = /DDO_WIN32 /DCONSOLE /DWIN32 /D "_MBCS" /D_CRT_SECURE_NO_WARNINGS /DVMAJOR=$(VERSION_MAJOR) /DVMINOR=$(VERSION_MINOR) /I "$(GSLPATH)"
LD = link.exe
LDFLAGS = /nologo /subsystem:console /incremental:no /opt:ref /opt:icf,5 /libpath:"$(GSLPATH)" /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib
LDLIBS = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib msvcrt.lib vitess.lib libgsl.lib

!ifndef NOG2
CPPFLAGS = $(CPPFLAGS) /DDO_PS /DVT_GRAPH /I "$(G2PATH)\src" /I "$(G2PATH)\src\Win32" /I "$(G2PATH)\src\PS"
LDFLAGS = $(LDFLAGS) /libpath:"$(G2PATH)\src"
LDLIBS = $(LDLIBS) libg2.lib
!endif

!ifdef DEBUG
CCFLAGS = $(CCFLAGS) /Od
CPPFLAGS = $(CPPFLAGS) /DDEBUG=1
!else
CCFLAGS = $(CCFLAGS) /Ox /Oy
CPPFLAGS = $(CPPFLAGS) /DNDEBUG
!endif

# Vitess modules to build
ALL = ascii2bin.exe \
beamstop.exe \
bender.exe \
capture_flux.exe \
cas_v40.exe \
chop_phases.exe \
chopper_disc.exe \
chopper_fermi.exe \
chopper_fermi_parallel.exe \
collimator.exe \
collimator_radial.exe \
collimator_soller.exe \
define_direction.exe \
detector.exe \
direct_view.exe \
eval_elast.exe \
eval_elast2.exe \
eval_inelast.exe \
eval_sans.exe \
filter.exe \
filter2D.exe \
flipper_coil.exe \
flipper_gradient.exe \
fom.exe \
frame.exe \
gener_batch.exe \
gener_bispectral.exe \
gener_hkl.exe \
gener_pipe.exe \
grid.exe \
guide.exe \
guide_elliptic.exe \
guide_parallel.exe \
guide_shape.exe \
kdsource.exe \
lattice_dist.exe \
lenses.exe \
merge.exe \
merge_spectra.exe \
mirror_coating.exe \
mirror_elliptical.exe \
mon1_brl.exe \
mon1_pol.exe \
mon2_div.exe \
mon2_kdiv.exe \
mon2_pos.exe \
mon2_posdiv.exe \
mon2_rdiv.exe \
mon2_tofwl.exe \
mon2_wldiv.exe \
mon_brilliance.exe \
monitor1.exe \
monitor1D.exe \
monitor2D.exe \
monitorpol_1d.exe \
monitorpol_pos.exe \
monochr_analyser.exe \
monochromator.exe \
opt_sim.exe \
pol_mirror.exe \
polariser_he3.exe \
polariser_sm.exe \
precessionfield.exe \
prism.exe \
read_in.exe \
resonator_drabkin.exe \
rotating_field.exe \
runtime.exe \
sample_elasticisotr.exe \
sample_environment.exe \
sample_inelast.exe \
sample_ncrystal.exe \
sample_nxs.exe \
sample_powder.exe \
sample_reflectom.exe \
sample_s_q.exe \
sample_sans.exe \
sample_singcryst.exe \
screen.exe \
sesans_field.exe \
slit.exe \
sm_ensemble.exe \
sm_ensemble_parallel.exe \
sortiap.exe \
source.exe \
space.exe \
spacewindow.exe \
spacewindow_multiple.exe \
spin_reset.exe \
standard_deviation.exe \
surface_file.exe \
velselect.exe \
window.exe \
writeout.exe
!ifndef NOG2
ALL = $(ALL) visual.exe dist_time.exe
!endif
!ifdef LIBTORCHPATH
ALL = $(ALL) source_ai.exe
!endif

# objects used by multiple modules
COMMON = init.obj \
general.obj \
convert.obj \
message.obj \
softabort.obj \
intersection.obj \
matrix.obj \
mathfunctions.obj \
sample.obj \
mathvector.obj \
mathmatrix.obj \
mcpl.obj \
mon2_header.obj \
trace.obj \
threadHelper.obj \
bender_inter_data.obj \
sswread.obj
!ifndef NOG2
COMMON = $(COMMON) cpgplot.obj
!endif

DEPS = $(GSLPATH)\libgsl.lib
!ifndef NOG2
DEPS = $(DEPS) $(G2PATH)\src\libg2.lib
!endif

# general make targets
all: $(DEPS) vitess.lib $(ALL)

clean:
	-del *.lib *.obj $(ALL)

distclean: clean
	cd "$(GSLPATH)"
	$(MAKE) /f gsl.mak clean
	cd "$(MAKEDIR)"
	cd "$(G2PATH)\src"
	$(MAKE) /f g2.mak clean
	cd "$(MAKEDIR)"

install: $(ALL)
	copy /Y *.exe ..\MODULES\\

vitess.lib: $(COMMON)
	lib /OUT:vitess.lib $(COMMON)

$(GSLPATH)\libgsl.lib:
	cd "$(GSLPATH)"
	$(MAKE) /f gsl.mak
	cd "$(MAKEDIR)"

$(G2PATH)\src\libg2.lib:
	cd "$(G2PATH)\src"
	$(MAKE) /f g2.mak
	cd "$(MAKEDIR)"

# KDSource for module kdsource
kdsource.exe: kdsource.c
	$(CC) $(CFLAGS) $(CPPFLAGS) /I "$(KDSOURCEPATH)\include" /Fe$@ $** $(LDLIBS) kdsource.lib bcrypt.lib /link $(LDFLAGS) /libpath:"$(KDSOURCEPATH)\lib"

# NCrystal for sample_ncrystal
sample_ncrystal.exe: sample_ncrystal.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) /I "$(NCRYSTALPATH)\include" /Fe$@ $** $(LDLIBS) NCrystal.lib /link $(LDFLAGS) /libpath:"$(NCRYSTALPATH)\lib"

source_ai.exe: source_ai.cpp
	$(CXX) $(CXXFLAGS) /wd4251 $(CPPFLAGS) /I "$(LIBTORCHPATH)\include" /I "$(LIBTORCHPATH)\include\torch\csrc\api\include" /Fe$@ $** $(LDLIBS) /link $(LDFLAGS) torch.lib torch_cpu.lib c10.lib /libpath:"$(LIBTORCHPATH)\lib"

# additional dependencies
bender.exe: bender.c bendtest.c bendchtr.c bendertr.c bender_inter_data.c

chopper_disc.exe: chopper_disc.c bender_inter_data.c

gener_batch.exe: gener_batch.c gener_fct.c

gener_pipe.exe: gener_pipe.c pipe_fct.c

grid.exe: grid.c bender_inter_data.c

lenses.exe: lenses.c lensetr.c

mirror_elliptical.exe: mirror_elliptical.c mirrrefl.c

monitor1D.exe: monitor1D.cpp mon1D.cpp

monitor2D.exe: monitor2D.cpp mon2D.cpp

monochr_analyser.exe: monochr_analyser.c ma_functions.c ma_geom.c

monochromator.exe: monochromator.cpp monochrclass.cpp

opt_sim.exe: opt_sim.c opt_grad.c opt_grad_mc.c opt_metro.c opt_swarm.c opt_fct.c calc_sim_fom.c

read_in.exe: read_in.c random_sampler.c

sample_nxs.exe: sample_nxs.c nxs.c sgclib.c sgfind.c sghkl.c sgio.c sgsi.c

sample_s_q.exe: sample_s_q.c sq_calc.c

source.exe: source.c src_modchar.c source_csns.c source_ess.c

window.exe: window.c bender_inter_data.c

spacewindow_multiple.exe: spacewindow_multiple.c bender_inter_data.c


# keep old module names for compatibility
chopper_fermi_parallel.exe: chopper_fermi.exe
	copy "$**" "$@"

guide_parallel.exe: guide.exe
	copy "$**" "$@"

mon_brilliance.exe: mon1_brl.exe
	copy "$**" "$@"

monitorpol_1d.exe: mon1_pol.exe
	copy "$**" "$@"

sm_ensemble_parallel.exe: sm_ensemble.exe
	copy "$**" "$@"

spacewindow.exe: window.exe
	copy "$**" "$@"


# suffix rules
.SUFFIXES: .c .cpp .obj .exe

.c.obj:
	$(CC) $(CFLAGS) $(CPPFLAGS) /c /Fo$@ $**

.cpp.obj:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) /c /Fo$@ $**

.c.exe:
	$(CC) $(CFLAGS) $(CPPFLAGS) /Fe$@ $** $(LDLIBS) /link $(LDFLAGS)

.cpp.exe:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) /Fe$@ $** $(LDLIBS) /link $(LDFLAGS)

.obj.exe:
	$(LD) $(LDLIBS) $(LDFLAGS) /OUT:$@ $**


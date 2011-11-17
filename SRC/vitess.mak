# Vitess NMAKE File
CPATH=c:\programme\microsoft visual studio .net 2003\vc7
SROOT=h:\control
SVNROOT=d:\vitcsrc
CPATH2=$(CPATH)\PlatformSDK
IPATH=$(CPATH)\include
LPATH=$(CPATH)\lib
IPATH2=$(CPATH2)\include
LPATH2=$(CPATH2)\lib

SPATH=$(SVNROOT)\SRC
GPATH=$(SROOT)\g2_win
GSLPATH=$(SPATH)\rng

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF

OD=.\Release
IDIR=.\Release

CPP=cl.exe
DEFS=/DNDEBUG /DDO_WIN32 /DCONSOLE /DWIN32 /D "_MBCS"
INC=/I "$(IPATH)" /I "$(IPATH2)" /I "$(SPATH)" /I "$(GSLPATH)"
CPP_OPT=/nologo /MT /W3 /Ox /Oy /Og /GF $(INC) $(DEFS) /Fp"$(IDIR)\vit.pch" /YX /FD /EHsc /c
CPP_PROJ=$(CPP_OPT) /Fo"$(IDIR)\\" /Fd"$(IDIR)\\"
GRAOPT=/I "$(GPATH)" /I "$(GPATH)\WIN32" /I "$(GPATH)\PS" /DDO_PS /DVT_GRAPH
LIBGSL=libgsl.lib

LINK32=link.exe
WINLIBS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib
LINK32_FLAGS=/nologo /subsystem:console /incremental:no /machine:I386 /opt:ref /opt:icf,5 \
 /libpath:"$(LPATH)" /libpath:"$(LPATH2)" /libpath:"$(GPATH)" /libpath:"$(GSLPATH)"
TOOL="$(IDIR)\init.obj" "$(IDIR)\general.obj" "$(IDIR)\message.obj" "$(IDIR)\softabort.obj"
ITOOL="$(IDIR)\intersection.obj" $(TOOL)
MTOOL="$(IDIR)\matrix.obj" $(ITOOL)
NTOOL="$(IDIR)\mathvector.obj" "$(IDIR)\mathmatrix.obj" "$(IDIR)\mon2D.obj" $(ITOOL)
MGTOOL=$(MTOOL)
STOOL="$(IDIR)\sample.obj" $(MTOOL)
GRALIB=g2.lib

ML=$(LIBGSL) $(WINLIBS) libcmt.lib /NODEFAULTLIB:libc.lib $(LINK32_FLAGS)
ML_T=$(LIBGSL) $(WINLIBS) libcmt.lib /NODEFAULTLIB:libc.lib $(LINK32_FLAGS)

.c{$(IDIR)}.obj::
 $(CPP) @<<
 $(CPP_PROJ) $<
<<

.cpp{$(IDIR)}.obj::
 $(CPP) @<<
 $(CPP_PROJ) $<
<<

"$(OD)" :
 if not exist "$(OD)\$(NULL)" mkdir "$(OD)"

ALL : \
	"$(OD)\ascii2bin.exe" \
	"$(OD)\monitor1.exe" \
	"$(OD)\mon2_div.exe" \
	"$(OD)\mon2_pos.exe" \
	"$(OD)\mon2_posdiv.exe" \
	"$(OD)\mon2_tofwl.exe" \
	"$(OD)\mon2_wldiv.exe" \
	"$(OD)\mon2_kdiv.exe" \
	"$(OD)\mon2_rdiv.exe" \
	"$(OD)\velselect.exe" \
	"$(OD)\writeout.exe" \
	"$(OD)\gener_batch.exe" \
	"$(OD)\lattice_dist.exe" \
	"$(OD)\mirror_coating.exe" \
	"$(OD)\surface_file.exe" \
	"$(OD)\guide_shape.exe" \
	"$(OD)\spin_reset.exe" \
	"$(OD)\capture_flux.exe" \
	"$(OD)\runtime.exe" \
	"$(OD)\chopper_disc.exe" \
	"$(OD)\chopper_fermi.exe" \
	"$(OD)\chopper_fermi_parallel.exe" \
	"$(OD)\collimator_soller.exe" \
	"$(OD)\collimator.exe" \
	"$(OD)\slit.exe" \
	"$(OD)\grid.exe" \
	"$(OD)\source.exe" \
	"$(OD)\spacewindow.exe" \
	"$(OD)\spacewindow_multiple.exe" \
	"$(OD)\space.exe" \
	"$(OD)\lenses.exe" \
	"$(OD)\beamstop.exe" \
	"$(OD)\detector.exe" \
	"$(OD)\eval_elast.exe" \
	"$(OD)\eval_elast2.exe" \
	"$(OD)\eval_inelast.exe" \
	"$(OD)\eval_sans.exe" \
	"$(OD)\frame.exe" \
	"$(OD)\guide.exe" \
	"$(OD)\guide_parallel.exe" \
	"$(OD)\monitorpol_1d.exe" \
	"$(OD)\monitorpol_pos.exe" \
	"$(OD)\monochr_analyser.exe" \
	"$(OD)\polariser_sm.exe" \
	"$(OD)\polariser_sm_parallel.exe" \
	"$(OD)\polariser_he3.exe" \
	"$(OD)\flipper_coil.exe" \
	"$(OD)\pol_mirror.exe" \
	"$(OD)\collimator_radial.exe" \
	"$(OD)\precessionfield.exe" \
	"$(OD)\sesans_field.exe" \
	"$(OD)\sample_elasticisotr.exe" \
	"$(OD)\sample_inelast.exe" \
	"$(OD)\sample_reflectom.exe" \
	"$(OD)\define_direction.exe" \
	"$(OD)\sample_singcryst.exe" \
	"$(OD)\cas_v40.exe" \
	"$(OD)\mirror_elliptical.exe" \
	"$(OD)\monitor2D.exe" \
	"$(OD)\rotating_field.exe" \
	"$(OD)\flipper_gradient.exe" \
	"$(OD)\resonator_drabkin.exe" \
	"$(OD)\sample_powder.exe" \
	"$(OD)\sample_s_q.exe" \
	"$(OD)\sample_sans.exe" \
	"$(OD)\sample_environment.exe" \
	"$(OD)\sample_nxs.exe" \
	"$(OD)\bender.exe" \
	"$(OD)\visual.exe" \
	"$(OD)\sm_ensemble.exe" \
	"$(OD)\sm_ensemble_parallel.exe" \
	"$(OD)\dist_time.exe" \
	"$(OD)\chop_phases.exe" \
	"$(OD)\standard_deviation.exe" \
	"$(OD)\direct_view.exe"

SOURCE=$(SPATH)\init.c
$(IDIR)\init.obj" : $(SOURCE)
	$(CPP) /DVVERS='"2.11"' $(CPP_PROJ) $(SOURCE)
SOURCE=$(SPATH)\general.c
"$(IDIR)\general.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\intersection.c
"$(IDIR)\intersection.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\matrix.c
"$(IDIR)\matrix.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\sample.c
"$(IDIR)\sample.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\softabort.c
"$(IDIR)\softabort.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\gener_fct.c
"$(IDIR)\gener_fct.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\read_table-lib.c
"$(IDIR)\read_table-lib.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\mirrrefl.c
"$(IDIR)\mirrrefl.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\lensetr.c
"$(IDIR)\lensetr.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\magneticmap.c
"$(IDIR)\magneticmap.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\sgio.c
"$(IDIR)\sgio.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\sgfind.c
"$(IDIR)\sgfind.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\sq_calc.c
"$(IDIR)\sq_calc.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\ma_functions.c
"$(IDIR)\ma_functions.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\sgsi.c
"$(IDIR)\sgsi.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\nxs.c
"$(IDIR)\nxs.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\bender_inter_data.c
"$(IDIR)\bender_inter_data.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\sghkl.c
"$(IDIR)\sghkl.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\threadHelper.c
"$(IDIR)\threadHelper.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\src_modchar.c
"$(IDIR)\src_modchar.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\sgclib.c
"$(IDIR)\sgclib.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\ma_geom.c
"$(IDIR)\ma_geom.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\cpgplot.c
"$(IDIR)\cpgplot.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\ascii2bin.c
"$(IDIR)\ascii2bin.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\ascii2bin.exe" : "$(OD)" $(TOOL) "$(OD)\ascii2bin.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\ascii2bin.pdb" /out:"$(OD)\ascii2bin.exe" "$(IDIR)\ascii2bin.obj" $(TOOL) 

SOURCE=$(SPATH)\monitor1.c
"$(IDIR)\monitor1.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\monitor1.exe" : "$(OD)" $(TOOL) "$(OD)\monitor1.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\monitor1.pdb" /out:"$(OD)\monitor1.exe" "$(IDIR)\monitor1.obj" $(TOOL) 

SOURCE=$(SPATH)\mon2_div.c
"$(IDIR)\mon2_div.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\mon2_div.exe" : "$(OD)" $(TOOL) "$(OD)\mon2_div.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\mon2_div.pdb" /out:"$(OD)\mon2_div.exe" "$(IDIR)\mon2_div.obj" $(TOOL) 

SOURCE=$(SPATH)\mon2_pos.c
"$(IDIR)\mon2_pos.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\mon2_pos.exe" : "$(OD)" $(TOOL) "$(OD)\mon2_pos.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\mon2_pos.pdb" /out:"$(OD)\mon2_pos.exe" "$(IDIR)\mon2_pos.obj" $(TOOL) 

SOURCE=$(SPATH)\mon2_posdiv.c
"$(IDIR)\mon2_posdiv.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\mon2_posdiv.exe" : "$(OD)" $(TOOL) "$(OD)\mon2_posdiv.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\mon2_posdiv.pdb" /out:"$(OD)\mon2_posdiv.exe" "$(IDIR)\mon2_posdiv.obj" $(TOOL) 

SOURCE=$(SPATH)\mon2_tofwl.c
"$(IDIR)\mon2_tofwl.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\mon2_tofwl.exe" : "$(OD)" $(TOOL) "$(OD)\mon2_tofwl.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\mon2_tofwl.pdb" /out:"$(OD)\mon2_tofwl.exe" "$(IDIR)\mon2_tofwl.obj" $(TOOL) 

SOURCE=$(SPATH)\mon2_wldiv.c
"$(IDIR)\mon2_wldiv.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\mon2_wldiv.exe" : "$(OD)" $(TOOL) "$(OD)\mon2_wldiv.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\mon2_wldiv.pdb" /out:"$(OD)\mon2_wldiv.exe" "$(IDIR)\mon2_wldiv.obj" $(TOOL) 

SOURCE=$(SPATH)\mon2_kdiv.c
"$(IDIR)\mon2_kdiv.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\mon2_kdiv.exe" : "$(OD)" $(TOOL) "$(OD)\mon2_kdiv.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\mon2_kdiv.pdb" /out:"$(OD)\mon2_kdiv.exe" "$(IDIR)\mon2_kdiv.obj" $(TOOL) 

SOURCE=$(SPATH)\mon2_rdiv.c
"$(IDIR)\mon2_rdiv.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\mon2_rdiv.exe" : "$(OD)" $(TOOL) "$(OD)\mon2_rdiv.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\mon2_rdiv.pdb" /out:"$(OD)\mon2_rdiv.exe" "$(IDIR)\mon2_rdiv.obj" $(TOOL) 

SOURCE=$(SPATH)\velselect.c
"$(IDIR)\velselect.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\velselect.exe" : "$(OD)" $(TOOL) "$(OD)\velselect.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\velselect.pdb" /out:"$(OD)\velselect.exe" "$(IDIR)\velselect.obj" $(TOOL) 

SOURCE=$(SPATH)\writeout.c
"$(IDIR)\writeout.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\writeout.exe" : "$(OD)" $(TOOL) "$(OD)\writeout.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\writeout.pdb" /out:"$(OD)\writeout.exe" "$(IDIR)\writeout.obj" $(TOOL) 

SOURCE=$(SPATH)\gener_batch.c
"$(IDIR)\gener_batch.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\gener_batch.exe" : "$(OD)" $(TOOL) "$(OD)\gener_batch.obj" "$(OD)\gener_fct.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\gener_batch.pdb" /out:"$(OD)\gener_batch.exe" "$(IDIR)\gener_batch.obj" $(TOOL) "$(OD)\gener_fct.obj" 

SOURCE=$(SPATH)\lattice_dist.c
"$(IDIR)\lattice_dist.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\lattice_dist.exe" : "$(OD)" $(TOOL) "$(OD)\lattice_dist.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\lattice_dist.pdb" /out:"$(OD)\lattice_dist.exe" "$(IDIR)\lattice_dist.obj" $(TOOL) 

SOURCE=$(SPATH)\mirror_coating.c
"$(IDIR)\mirror_coating.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\mirror_coating.exe" : "$(OD)" $(TOOL) "$(OD)\mirror_coating.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\mirror_coating.pdb" /out:"$(OD)\mirror_coating.exe" "$(IDIR)\mirror_coating.obj" $(TOOL) 

SOURCE=$(SPATH)\surface_file.c
"$(IDIR)\surface_file.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\surface_file.exe" : "$(OD)" $(TOOL) "$(OD)\surface_file.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\surface_file.pdb" /out:"$(OD)\surface_file.exe" "$(IDIR)\surface_file.obj" $(TOOL) 

SOURCE=$(SPATH)\guide_shape.c
"$(IDIR)\guide_shape.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\guide_shape.exe" : "$(OD)" $(TOOL) "$(OD)\guide_shape.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\guide_shape.pdb" /out:"$(OD)\guide_shape.exe" "$(IDIR)\guide_shape.obj" $(TOOL) 

SOURCE=$(SPATH)\spin_reset.c
"$(IDIR)\spin_reset.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\spin_reset.exe" : "$(OD)" $(TOOL) "$(OD)\spin_reset.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\spin_reset.pdb" /out:"$(OD)\spin_reset.exe" "$(IDIR)\spin_reset.obj" $(TOOL) 

SOURCE=$(SPATH)\capture_flux.c
"$(IDIR)\capture_flux.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\capture_flux.exe" : "$(OD)" $(TOOL) "$(OD)\capture_flux.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\capture_flux.pdb" /out:"$(OD)\capture_flux.exe" "$(IDIR)\capture_flux.obj" $(TOOL) 

SOURCE=$(SPATH)\runtime.c
"$(IDIR)\runtime.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\runtime.exe" : "$(OD)" $(TOOL) "$(OD)\runtime.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\runtime.pdb" /out:"$(OD)\runtime.exe" "$(IDIR)\runtime.obj" $(TOOL) 

SOURCE=$(SPATH)\chopper_disc.c
"$(IDIR)\chopper_disc.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\chopper_disc.exe" : "$(OD)" $(ITOOL) "$(OD)\chopper_disc.obj" "$(OD)\bender_inter_data.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\chopper_disc.pdb" /out:"$(OD)\chopper_disc.exe" "$(IDIR)\chopper_disc.obj" $(ITOOL) "$(OD)\bender_inter_data.obj" 

SOURCE=$(SPATH)\chopper_fermi.c
"$(IDIR)\chopper_fermi.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\chopper_fermi.exe" : "$(OD)" $(ITOOL) "$(OD)\chopper_fermi.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\chopper_fermi.pdb" /out:"$(OD)\chopper_fermi.exe" "$(IDIR)\chopper_fermi.obj" $(ITOOL) 

SOURCE=$(SPATH)\chopper_fermi_parallel.c
"$(IDIR)\chopper_fermi_parallel.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\chopper_fermi_parallel.exe" : "$(OD)" $(ITOOL) "$(OD)\chopper_fermi_parallel.obj" "$(OD)\threadHelper.obj"
	$(LINK32) $(ML_T) /pdb:"$(OD)\chopper_fermi_parallel.pdb" /out:"$(OD)\chopper_fermi_parallel.exe" "$(IDIR)\chopper_fermi_parallel.obj" $(ITOOL) "$(OD)\threadHelper.obj" 

SOURCE=$(SPATH)\collimator_soller.c
"$(IDIR)\collimator_soller.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\collimator_soller.exe" : "$(OD)" $(ITOOL) "$(OD)\collimator_soller.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\collimator_soller.pdb" /out:"$(OD)\collimator_soller.exe" "$(IDIR)\collimator_soller.obj" $(ITOOL) 

SOURCE=$(SPATH)\collimator.c
"$(IDIR)\collimator.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\collimator.exe" : "$(OD)" $(ITOOL) "$(OD)\collimator.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\collimator.pdb" /out:"$(OD)\collimator.exe" "$(IDIR)\collimator.obj" $(ITOOL) 

SOURCE=$(SPATH)\slit.c
"$(IDIR)\slit.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\slit.exe" : "$(OD)" $(ITOOL) "$(OD)\slit.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\slit.pdb" /out:"$(OD)\slit.exe" "$(IDIR)\slit.obj" $(ITOOL) 

SOURCE=$(SPATH)\grid.c
"$(IDIR)\grid.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\grid.exe" : "$(OD)" $(ITOOL) "$(OD)\grid.obj" "$(OD)\bender_inter_data.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\grid.pdb" /out:"$(OD)\grid.exe" "$(IDIR)\grid.obj" $(ITOOL) "$(OD)\bender_inter_data.obj" 

SOURCE=$(SPATH)\source.c
"$(IDIR)\source.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\source.exe" : "$(OD)" $(ITOOL) "$(OD)\source.obj" "$(OD)\src_modchar.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\source.pdb" /out:"$(OD)\source.exe" "$(IDIR)\source.obj" $(ITOOL) "$(OD)\src_modchar.obj" 

SOURCE=$(SPATH)\spacewindow.c
"$(IDIR)\spacewindow.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\spacewindow.exe" : "$(OD)" $(ITOOL) "$(OD)\spacewindow.obj" "$(OD)\bender_inter_data.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\spacewindow.pdb" /out:"$(OD)\spacewindow.exe" "$(IDIR)\spacewindow.obj" $(ITOOL) "$(OD)\bender_inter_data.obj" 

SOURCE=$(SPATH)\spacewindow_multiple.c
"$(IDIR)\spacewindow_multiple.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\spacewindow_multiple.exe" : "$(OD)" $(ITOOL) "$(OD)\spacewindow_multiple.obj" "$(OD)\bender_inter_data.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\spacewindow_multiple.pdb" /out:"$(OD)\spacewindow_multiple.exe" "$(IDIR)\spacewindow_multiple.obj" $(ITOOL) "$(OD)\bender_inter_data.obj" 

SOURCE=$(SPATH)\space.c
"$(IDIR)\space.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\space.exe" : "$(OD)" $(ITOOL) "$(OD)\space.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\space.pdb" /out:"$(OD)\space.exe" "$(IDIR)\space.obj" $(ITOOL) 

SOURCE=$(SPATH)\lenses.c
"$(IDIR)\lenses.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\lenses.exe" : "$(OD)" $(ITOOL) "$(OD)\lenses.obj" "$(OD)\lensetr.obj" "$(OD)\cpgplot.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\lenses.pdb" /out:"$(OD)\lenses.exe" "$(IDIR)\lenses.obj" $(ITOOL) "$(OD)\lensetr.obj" "$(OD)\cpgplot.obj"  $(GRALIB)

SOURCE=$(SPATH)\beamstop.c
"$(IDIR)\beamstop.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\beamstop.exe" : "$(OD)" $(ITOOL) "$(OD)\beamstop.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\beamstop.pdb" /out:"$(OD)\beamstop.exe" "$(IDIR)\beamstop.obj" $(ITOOL) 

SOURCE=$(SPATH)\detector.c
"$(IDIR)\detector.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\detector.exe" : "$(OD)" $(MTOOL) "$(OD)\detector.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\detector.pdb" /out:"$(OD)\detector.exe" "$(IDIR)\detector.obj" $(MTOOL) 

SOURCE=$(SPATH)\eval_elast.c
"$(IDIR)\eval_elast.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\eval_elast.exe" : "$(OD)" $(MTOOL) "$(OD)\eval_elast.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\eval_elast.pdb" /out:"$(OD)\eval_elast.exe" "$(IDIR)\eval_elast.obj" $(MTOOL) 

SOURCE=$(SPATH)\eval_elast2.c
"$(IDIR)\eval_elast2.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\eval_elast2.exe" : "$(OD)" $(MTOOL) "$(OD)\eval_elast2.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\eval_elast2.pdb" /out:"$(OD)\eval_elast2.exe" "$(IDIR)\eval_elast2.obj" $(MTOOL) 

SOURCE=$(SPATH)\eval_inelast.c
"$(IDIR)\eval_inelast.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\eval_inelast.exe" : "$(OD)" $(MTOOL) "$(OD)\eval_inelast.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\eval_inelast.pdb" /out:"$(OD)\eval_inelast.exe" "$(IDIR)\eval_inelast.obj" $(MTOOL) 

SOURCE=$(SPATH)\eval_sans.c
"$(IDIR)\eval_sans.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\eval_sans.exe" : "$(OD)" $(MTOOL) "$(OD)\eval_sans.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\eval_sans.pdb" /out:"$(OD)\eval_sans.exe" "$(IDIR)\eval_sans.obj" $(MTOOL) 

SOURCE=$(SPATH)\frame.c
"$(IDIR)\frame.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\frame.exe" : "$(OD)" $(MTOOL) "$(OD)\frame.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\frame.pdb" /out:"$(OD)\frame.exe" "$(IDIR)\frame.obj" $(MTOOL) 

SOURCE=$(SPATH)\guide.c
"$(IDIR)\guide.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\guide.exe" : "$(OD)" $(MTOOL) "$(OD)\guide.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\guide.pdb" /out:"$(OD)\guide.exe" "$(IDIR)\guide.obj" $(MTOOL) 

SOURCE=$(SPATH)\guide_parallel.c
"$(IDIR)\guide_parallel.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\guide_parallel.exe" : "$(OD)" $(MTOOL) "$(OD)\guide_parallel.obj" "$(OD)\threadHelper.obj"
	$(LINK32) $(ML_T) /pdb:"$(OD)\guide_parallel.pdb" /out:"$(OD)\guide_parallel.exe" "$(IDIR)\guide_parallel.obj" $(MTOOL) "$(OD)\threadHelper.obj" 

SOURCE=$(SPATH)\monitorpol_1d.c
"$(IDIR)\monitorpol_1d.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\monitorpol_1d.exe" : "$(OD)" $(MTOOL) "$(OD)\monitorpol_1d.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\monitorpol_1d.pdb" /out:"$(OD)\monitorpol_1d.exe" "$(IDIR)\monitorpol_1d.obj" $(MTOOL) 

SOURCE=$(SPATH)\monitorpol_pos.c
"$(IDIR)\monitorpol_pos.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\monitorpol_pos.exe" : "$(OD)" $(MTOOL) "$(OD)\monitorpol_pos.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\monitorpol_pos.pdb" /out:"$(OD)\monitorpol_pos.exe" "$(IDIR)\monitorpol_pos.obj" $(MTOOL) 

SOURCE=$(SPATH)\monochr_analyser.c
"$(IDIR)\monochr_analyser.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\monochr_analyser.exe" : "$(OD)" $(MTOOL) "$(OD)\monochr_analyser.obj" "$(OD)\ma_functions.obj" "$(OD)\ma_geom.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\monochr_analyser.pdb" /out:"$(OD)\monochr_analyser.exe" "$(IDIR)\monochr_analyser.obj" $(MTOOL) "$(OD)\ma_functions.obj" "$(OD)\ma_geom.obj" 

SOURCE=$(SPATH)\polariser_sm.c
"$(IDIR)\polariser_sm.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\polariser_sm.exe" : "$(OD)" $(MTOOL) "$(OD)\polariser_sm.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\polariser_sm.pdb" /out:"$(OD)\polariser_sm.exe" "$(IDIR)\polariser_sm.obj" $(MTOOL) 

SOURCE=$(SPATH)\polariser_sm_parallel.c
"$(IDIR)\polariser_sm_parallel.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\polariser_sm_parallel.exe" : "$(OD)" $(MTOOL) "$(OD)\polariser_sm_parallel.obj" "$(OD)\threadHelper.obj"
	$(LINK32) $(ML_T) /pdb:"$(OD)\polariser_sm_parallel.pdb" /out:"$(OD)\polariser_sm_parallel.exe" "$(IDIR)\polariser_sm_parallel.obj" $(MTOOL) "$(OD)\threadHelper.obj" 

SOURCE=$(SPATH)\polariser_he3.c
"$(IDIR)\polariser_he3.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\polariser_he3.exe" : "$(OD)" $(MTOOL) "$(OD)\polariser_he3.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\polariser_he3.pdb" /out:"$(OD)\polariser_he3.exe" "$(IDIR)\polariser_he3.obj" $(MTOOL) 

SOURCE=$(SPATH)\flipper_coil.c
"$(IDIR)\flipper_coil.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\flipper_coil.exe" : "$(OD)" $(MTOOL) "$(OD)\flipper_coil.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\flipper_coil.pdb" /out:"$(OD)\flipper_coil.exe" "$(IDIR)\flipper_coil.obj" $(MTOOL) 

SOURCE=$(SPATH)\pol_mirror.c
"$(IDIR)\pol_mirror.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\pol_mirror.exe" : "$(OD)" $(MTOOL) "$(OD)\pol_mirror.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\pol_mirror.pdb" /out:"$(OD)\pol_mirror.exe" "$(IDIR)\pol_mirror.obj" $(MTOOL) 

SOURCE=$(SPATH)\collimator_radial.c
"$(IDIR)\collimator_radial.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\collimator_radial.exe" : "$(OD)" $(MTOOL) "$(OD)\collimator_radial.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\collimator_radial.pdb" /out:"$(OD)\collimator_radial.exe" "$(IDIR)\collimator_radial.obj" $(MTOOL) 

SOURCE=$(SPATH)\precessionfield.c
"$(IDIR)\precessionfield.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\precessionfield.exe" : "$(OD)" $(MTOOL) "$(OD)\precessionfield.obj" "$(OD)\magneticmap.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\precessionfield.pdb" /out:"$(OD)\precessionfield.exe" "$(IDIR)\precessionfield.obj" $(MTOOL) "$(OD)\magneticmap.obj" 

SOURCE=$(SPATH)\sesans_field.c
"$(IDIR)\sesans_field.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sesans_field.exe" : "$(OD)" $(MTOOL) "$(OD)\sesans_field.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sesans_field.pdb" /out:"$(OD)\sesans_field.exe" "$(IDIR)\sesans_field.obj" $(MTOOL) 

SOURCE=$(SPATH)\sample_elasticisotr.c
"$(IDIR)\sample_elasticisotr.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sample_elasticisotr.exe" : "$(OD)" $(MTOOL) "$(OD)\sample_elasticisotr.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sample_elasticisotr.pdb" /out:"$(OD)\sample_elasticisotr.exe" "$(IDIR)\sample_elasticisotr.obj" $(MTOOL) 

SOURCE=$(SPATH)\sample_inelast.c
"$(IDIR)\sample_inelast.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sample_inelast.exe" : "$(OD)" $(MTOOL) "$(OD)\sample_inelast.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sample_inelast.pdb" /out:"$(OD)\sample_inelast.exe" "$(IDIR)\sample_inelast.obj" $(MTOOL) 

SOURCE=$(SPATH)\sample_reflectom.c
"$(IDIR)\sample_reflectom.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sample_reflectom.exe" : "$(OD)" $(MTOOL) "$(OD)\sample_reflectom.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sample_reflectom.pdb" /out:"$(OD)\sample_reflectom.exe" "$(IDIR)\sample_reflectom.obj" $(MTOOL) 

SOURCE=$(SPATH)\define_direction.c
"$(IDIR)\define_direction.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\define_direction.exe" : "$(OD)" $(MTOOL) "$(OD)\define_direction.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\define_direction.pdb" /out:"$(OD)\define_direction.exe" "$(IDIR)\define_direction.obj" $(MTOOL) 

SOURCE=$(SPATH)\sample_singcryst.c
"$(IDIR)\sample_singcryst.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sample_singcryst.exe" : "$(OD)" $(MTOOL) "$(OD)\sample_singcryst.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sample_singcryst.pdb" /out:"$(OD)\sample_singcryst.exe" "$(IDIR)\sample_singcryst.obj" $(MTOOL) 

SOURCE=$(SPATH)\cas_v40.c
"$(IDIR)\cas_v40.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\cas_v40.exe" : "$(OD)" $(MTOOL) "$(OD)\cas_v40.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\cas_v40.pdb" /out:"$(OD)\cas_v40.exe" "$(IDIR)\cas_v40.obj" $(MTOOL) 

SOURCE=$(SPATH)\mirror_elliptical.c
"$(IDIR)\mirror_elliptical.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\mirror_elliptical.exe" : "$(OD)" $(MTOOL) "$(OD)\mirror_elliptical.obj" "$(OD)\mirrrefl.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\mirror_elliptical.pdb" /out:"$(OD)\mirror_elliptical.exe" "$(IDIR)\mirror_elliptical.obj" $(MTOOL) "$(OD)\mirrrefl.obj" 

SOURCE=$(SPATH)\monitor2D.cpp
"$(IDIR)\monitor2D.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\monitor2D.exe" : "$(OD)" $(NTOOL) "$(OD)\monitor2D.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\monitor2D.pdb" /out:"$(OD)\monitor2D.exe" "$(IDIR)\monitor2D.obj" $(NTOOL) 

SOURCE=$(SPATH)\rotating_field.c
"$(IDIR)\rotating_field.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\rotating_field.exe" : "$(OD)" $(MGTOOL) "$(OD)\rotating_field.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\rotating_field.pdb" /out:"$(OD)\rotating_field.exe" "$(IDIR)\rotating_field.obj" $(MGTOOL) 

SOURCE=$(SPATH)\flipper_gradient.c
"$(IDIR)\flipper_gradient.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\flipper_gradient.exe" : "$(OD)" $(MGTOOL) "$(OD)\flipper_gradient.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\flipper_gradient.pdb" /out:"$(OD)\flipper_gradient.exe" "$(IDIR)\flipper_gradient.obj" $(MGTOOL) 

SOURCE=$(SPATH)\resonator_drabkin.c
"$(IDIR)\resonator_drabkin.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\resonator_drabkin.exe" : "$(OD)" $(MGTOOL) "$(OD)\resonator_drabkin.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\resonator_drabkin.pdb" /out:"$(OD)\resonator_drabkin.exe" "$(IDIR)\resonator_drabkin.obj" $(MGTOOL) 

SOURCE=$(SPATH)\sample_powder.c
"$(IDIR)\sample_powder.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sample_powder.exe" : "$(OD)" $(STOOL) "$(OD)\sample_powder.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sample_powder.pdb" /out:"$(OD)\sample_powder.exe" "$(IDIR)\sample_powder.obj" $(STOOL) 

SOURCE=$(SPATH)\sample_s_q.c
"$(IDIR)\sample_s_q.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sample_s_q.exe" : "$(OD)" $(STOOL) "$(OD)\sample_s_q.obj" "$(OD)\sq_calc.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sample_s_q.pdb" /out:"$(OD)\sample_s_q.exe" "$(IDIR)\sample_s_q.obj" $(STOOL) "$(OD)\sq_calc.obj" 

SOURCE=$(SPATH)\sample_sans.c
"$(IDIR)\sample_sans.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sample_sans.exe" : "$(OD)" $(STOOL) "$(OD)\sample_sans.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sample_sans.pdb" /out:"$(OD)\sample_sans.exe" "$(IDIR)\sample_sans.obj" $(STOOL) 

SOURCE=$(SPATH)\sample_environment.c
"$(IDIR)\sample_environment.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sample_environment.exe" : "$(OD)" $(STOOL) "$(OD)\sample_environment.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sample_environment.pdb" /out:"$(OD)\sample_environment.exe" "$(IDIR)\sample_environment.obj" $(STOOL) 

SOURCE=$(SPATH)\sample_nxs.c
"$(IDIR)\sample_nxs.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\sample_nxs.exe" : "$(OD)" $(STOOL) "$(OD)\sample_nxs.obj" "$(OD)\sghkl.obj" "$(OD)\sgclib.obj" "$(OD)\sgio.obj" "$(OD)\sgfind.obj" "$(OD)\read_table-lib.obj" "$(OD)\sgsi.obj" "$(OD)\nxs.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\sample_nxs.pdb" /out:"$(OD)\sample_nxs.exe" "$(IDIR)\sample_nxs.obj" $(STOOL) "$(OD)\sghkl.obj" "$(OD)\sgclib.obj" "$(OD)\sgio.obj" "$(OD)\sgfind.obj" "$(OD)\read_table-lib.obj" "$(OD)\sgsi.obj" "$(OD)\nxs.obj" 

SOURCE=$(SPATH)\chop_phases.c
"$(IDIR)\chop_phases.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\chop_phases.exe" : "$(OD)" "$(OD)\chop_phases.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\chop_phases.pdb" /out:"$(OD)\chop_phases.exe" "$(IDIR)\chop_phases.obj"

SOURCE=$(SPATH)\standard_deviation.c
"$(IDIR)\standard_deviation.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\standard_deviation.exe" : "$(OD)" "$(OD)\standard_deviation.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\standard_deviation.pdb" /out:"$(OD)\standard_deviation.exe" "$(IDIR)\standard_deviation.obj"

SOURCE=$(SPATH)\direct_view.c
"$(IDIR)\direct_view.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\direct_view.exe" : "$(OD)" "$(OD)\direct_view.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\direct_view.pdb" /out:"$(OD)\direct_view.exe" "$(IDIR)\direct_view.obj"

SOURCE=$(SPATH)\bendtest.c
"$(IDIR)\bendtest.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\bendchtr.c
"$(IDIR)\bendchtr.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\bendertr.c
"$(IDIR)\bendertr.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\bender.c
"$(IDIR)\bender.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

"$(OD)\bender.exe" : "$(OD)" "$(OD)\bender.obj" $(MTOOL) "$(OD)\bender_inter_data.obj" "$(OD)\bendchtr.obj" "$(OD)\cpgplot.obj" "$(OD)\bendtest.obj" "$(OD)\bendertr.obj"
	$(LINK32) $(ML) $(MTOOL) $(GRALIB) /pdb:"$(OD)\bender.pdb" /out:"$(OD)\bender.exe" "$(IDIR)\bender.obj" "$(OD)\bender_inter_data.obj" "$(OD)\bendchtr.obj" "$(OD)\cpgplot.obj" "$(OD)\bendtest.obj" "$(OD)\bendertr.obj"

SOURCE=$(SPATH)\visual.c
"$(IDIR)\visual.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

"$(OD)\visual.exe" : "$(OD)" "$(OD)\visual.obj" $(MTOOL) "$(OD)\cpgplot.obj"
	$(LINK32) $(ML) $(MTOOL) $(GRALIB) /pdb:"$(OD)\visual.pdb" /out:"$(OD)\visual.exe" "$(IDIR)\visual.obj" "$(OD)\cpgplot.obj"

SOURCE=$(SPATH)\sm_ensemble.c
"$(IDIR)\sm_ensemble.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

"$(OD)\sm_ensemble.exe" : "$(OD)" "$(OD)\sm_ensemble.obj" $(MTOOL) "$(OD)\cpgplot.obj"
	$(LINK32) $(ML) $(MTOOL) $(GRALIB) /pdb:"$(OD)\sm_ensemble.pdb" /out:"$(OD)\sm_ensemble.exe" "$(IDIR)\sm_ensemble.obj" "$(OD)\cpgplot.obj"

SOURCE=$(SPATH)\sm_ensemble_parallel.c
"$(IDIR)\sm_ensemble_parallel.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

"$(OD)\sm_ensemble_parallel.exe" : "$(OD)" "$(OD)\sm_ensemble_parallel.obj" $(MTOOL) "$(OD)\threadHelper.obj" "$(OD)\cpgplot.obj"
	$(LINK32) $(ML_T) $(MTOOL) $(GRALIB) /pdb:"$(OD)\sm_ensemble_parallel.pdb" /out:"$(OD)\sm_ensemble_parallel.exe" "$(IDIR)\sm_ensemble_parallel.obj" "$(OD)\threadHelper.obj" "$(OD)\cpgplot.obj"

SOURCE=$(SPATH)\dist_time.c
"$(IDIR)\dist_time.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

"$(OD)\dist_time.exe" : "$(OD)" "$(OD)\dist_time.obj" $(MTOOL) "$(OD)\cpgplot.obj"
	$(LINK32) $(ML) $(MTOOL) $(GRALIB) /pdb:"$(OD)\dist_time.pdb" /out:"$(OD)\dist_time.exe" "$(IDIR)\dist_time.obj" "$(OD)\cpgplot.obj"

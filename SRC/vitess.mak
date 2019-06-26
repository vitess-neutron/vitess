# Vitess NMAKE File
CPATH=c:\programme\microsoft visual studio .net 2003\vc7
SROOT=h:\control
SVNROOT=h:\control\vitess\trunk
CPATH2=$(CPATH)\PlatformSDK
IPATH=$(CPATH)\include
LPATH=$(CPATH)\lib
IPATH2=$(CPATH2)\include
LPATH2=$(CPATH2)\lib

SPATH=$(SVNROOT)\SRC
GPATH=$(SROOT)\g2_win

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF

OD=.\Release
IDIR=.\Release

CPP=cl.exe
DEFS=/DNDEBUG /DDO_WIN32 /DCONSOLE /DWIN32 /D "_MBCS"
INC=/I "$(IPATH)" /I "$(IPATH2)" /I "$(SPATH)"
CPP_OPT=/nologo /ML /W3 /Ox $(INC) $(DEFS) /Fp"$(IDIR)\vit.pch" /YX /FD /c
CPP_PROJ=$(CPP_OPT) /Fo"$(IDIR)\\" /Fd"$(IDIR)\\"
GRAOPT=/I "$(GPATH)" /I "$(GPATH)\WIN32" /I "$(GPATH)\PS" /DDO_PS /DVT_GRAPH

LINK32=link.exe
WINLIBS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib \
 shell32.lib
LINK32_FLAGS=/nologo /subsystem:console /incremental:no /machine:I386 /opt:ref /opt:icf,5 \
 /libpath:"$(LPATH)" /libpath:"$(LPATH2)" /libpath:"$(GPATH)"
TOOL="$(IDIR)\init.obj" "$(IDIR)\general.obj" "$(IDIR)\message.obj"
ITOOL="$(IDIR)\intersection.obj" $(TOOL)
MTOOL="$(IDIR)\matrix.obj" $(ITOOL)
STOOL="$(IDIR)\sample.obj" $(MTOOL)
GRALIB=g2.lib
ML=$(WINLIBS) $(LINK32_FLAGS)

.c{$(IDIR)}.obj::
 $(CPP) @<<
 $(CPP_PROJ) $<
<<

"$(OD)" :
 if not exist "$(OD)\$(NULL)" mkdir "$(OD)"

ALL : \
	"$(OD)\ascii2bin.exe" \
	"$(OD)\eval_elast.exe" \
	"$(OD)\monitor1.exe" \
	"$(OD)\mon2_div.exe" \
	"$(OD)\mon2_pos.exe" \
	"$(OD)\mon2_posdiv.exe" \
	"$(OD)\mon2_tofwl.exe" \
	"$(OD)\mon2_wldiv.exe" \
	"$(OD)\velselect.exe" \
	"$(OD)\writeout.exe" \
	"$(OD)\gener_batch.exe" \
	"$(OD)\lattice_dist.exe" \
	"$(OD)\mirror_coating.exe" \
	"$(OD)\surface_file.exe" \
	"$(OD)\guide_shape.exe" \
	"$(OD)\chopper_disc.exe" \
	"$(OD)\chopper_fermi.exe" \
	"$(OD)\collimator_soller.exe" \
	"$(OD)\source.exe" \
	"$(OD)\spacewindow.exe" \
	"$(OD)\spacewindow_multiple.exe" \
	"$(OD)\space.exe" \
	"$(OD)\detector.exe" \
	"$(OD)\eval_inelast.exe" \
	"$(OD)\frame.exe" \
	"$(OD)\guide.exe" \
	"$(OD)\monitorpol_1d.exe" \
	"$(OD)\monitorpol_pos.exe" \
	"$(OD)\monochr_analyser.exe" \
	"$(OD)\polariser_sm.exe" \
	"$(OD)\polariser_he3.exe" \
	"$(OD)\flipper_coil.exe" \
	"$(OD)\pol_mirror.exe" \
	"$(OD)\precessionfield.exe" \
	"$(OD)\rotating_field.exe" \
	"$(OD)\flipper_gradient.exe" \
	"$(OD)\resonator_drabkin.exe" \
	"$(OD)\sample_elasticisotr.exe" \
	"$(OD)\sample_inelast.exe" \
	"$(OD)\sample_reflectom.exe" \
	"$(OD)\define_direction.exe" \
	"$(OD)\sample_singcryst.exe" \
	"$(OD)\cas_v40.exe" \
	"$(OD)\sample_powder.exe" \
	"$(OD)\sample_s_q.exe" \
	"$(OD)\sample_sans.exe" \
	"$(OD)\bender.exe" \
	"$(OD)\visual.exe" \
	"$(OD)\sm_ensemble.exe" \
	"$(OD)\dist_time.exe" \
	"$(OD)\chop_phases.exe" \
	"$(OD)\standard_deviation.exe" \
	"$(OD)\direct_view.exe"

SOURCE=$(SPATH)\init.c
"$(IDIR)\init.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

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

SOURCE=$(SPATH)\bender_inter_data.c
"$(IDIR)\bender_inter_data.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\sq_calc.c
"$(IDIR)\sq_calc.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\ma_functions.c
"$(IDIR)\ma_functions.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\gener_fct.c
"$(IDIR)\gener_fct.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\ma_geom.c
"$(IDIR)\ma_geom.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\src_modchar.c
"$(IDIR)\src_modchar.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\magneticmap.c
"$(IDIR)\magneticmap.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\ascii2bin.c
"$(IDIR)\ascii2bin.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\ascii2bin.exe" : "$(OD)" $(TOOL) "$(OD)\ascii2bin.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\ascii2bin.pdb" /out:"$(OD)\ascii2bin.exe" "$(IDIR)\ascii2bin.obj" $(TOOL) 

SOURCE=$(SPATH)\eval_elast.c
"$(IDIR)\eval_elast.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\eval_elast.exe" : "$(OD)" $(TOOL) "$(OD)\eval_elast.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\eval_elast.pdb" /out:"$(OD)\eval_elast.exe" "$(IDIR)\eval_elast.obj" $(TOOL) 

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

SOURCE=$(SPATH)\collimator_soller.c
"$(IDIR)\collimator_soller.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\collimator_soller.exe" : "$(OD)" $(ITOOL) "$(OD)\collimator_soller.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\collimator_soller.pdb" /out:"$(OD)\collimator_soller.exe" "$(IDIR)\collimator_soller.obj" $(ITOOL) 

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

SOURCE=$(SPATH)\detector.c
"$(IDIR)\detector.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\detector.exe" : "$(OD)" $(MTOOL) "$(OD)\detector.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\detector.pdb" /out:"$(OD)\detector.exe" "$(IDIR)\detector.obj" $(MTOOL) 

SOURCE=$(SPATH)\eval_inelast.c
"$(IDIR)\eval_inelast.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\eval_inelast.exe" : "$(OD)" $(MTOOL) "$(OD)\eval_inelast.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\eval_inelast.pdb" /out:"$(OD)\eval_inelast.exe" "$(IDIR)\eval_inelast.obj" $(MTOOL) 

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

SOURCE=$(SPATH)\precessionfield.c
"$(IDIR)\precessionfield.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\precessionfield.exe" : "$(OD)" $(MTOOL) "$(OD)\precessionfield.obj" "$(OD)\magneticmap.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\precessionfield.pdb" /out:"$(OD)\precessionfield.exe" "$(IDIR)\precessionfield.obj" $(MTOOL) "$(OD)\magneticmap.obj" 

SOURCE=$(SPATH)\rotating_field.c
"$(IDIR)\rotating_field.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\rotating_field.exe" : "$(OD)" $(MTOOL) "$(OD)\rotating_field.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\rotating_field.pdb" /out:"$(OD)\rotating_field.exe" "$(IDIR)\rotating_field.obj" $(MTOOL) 

SOURCE=$(SPATH)\flipper_gradient.c
"$(IDIR)\flipper_gradient.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\flipper_gradient.exe" : "$(OD)" $(MTOOL) "$(OD)\flipper_gradient.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\flipper_gradient.pdb" /out:"$(OD)\flipper_gradient.exe" "$(IDIR)\flipper_gradient.obj" $(MTOOL) 

SOURCE=$(SPATH)\resonator_drabkin.c
"$(IDIR)\resonator_drabkin.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

"$(OD)\resonator_drabkin.exe" : "$(OD)" $(MTOOL) "$(OD)\resonator_drabkin.obj"
	$(LINK32) $(ML) /pdb:"$(OD)\resonator_drabkin.pdb" /out:"$(OD)\resonator_drabkin.exe" "$(IDIR)\resonator_drabkin.obj" $(MTOOL) 

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

SOURCE=$(SPATH)\cpgplot.c
"$(IDIR)\cpgplot.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

SOURCE=$(SPATH)\bender.c
"$(IDIR)\bender.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

"$(OD)\bender.exe" : "$(OD)" "$(OD)\bender.obj" $(MTOOL) "$(OD)\bendtest.obj" "$(OD)\bendchtr.obj" "$(OD)\bendertr.obj" "$(OD)\bender_inter_data.obj" "$(OD)\cpgplot.obj"
	$(LINK32) $(ML) $(MTOOL) $(GRALIB) /pdb:"$(OD)\bender.pdb" /out:"$(OD)\bender.exe" "$(IDIR)\bender.obj" "$(OD)\bendtest.obj" "$(OD)\bendchtr.obj" "$(OD)\bendertr.obj" "$(OD)\bender_inter_data.obj" "$(OD)\cpgplot.obj"

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

SOURCE=$(SPATH)\dist_time.c
"$(IDIR)\dist_time.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

"$(OD)\dist_time.exe" : "$(OD)" "$(OD)\dist_time.obj" $(MTOOL) "$(OD)\cpgplot.obj"
	$(LINK32) $(ML) $(MTOOL) $(GRALIB) /pdb:"$(OD)\dist_time.pdb" /out:"$(OD)\dist_time.exe" "$(IDIR)\dist_time.obj" "$(OD)\cpgplot.obj"

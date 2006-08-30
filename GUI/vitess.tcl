### project Xcontrol
### HMI DN
### M. Fromme fromme@hmi.de
### June 1999

### control variables lists and procedures for
### VITESS simulation
###
### list of all (static, always part of interface) input list structures
###
set globalDescriptionSET {
  inputESET xcontolDefaultsESET
}

#   Type names		        Extension(s)	Mac File Type(s)

set fileDialogSET {
  {"All files" *}
  {"GUI settings" {.gui}}
  {"Batch command files" {.bat}}
  {"Tcl files" {.tcl}}
  {"Series information files" {.inf}}
  {"X,Y ASCII files" {.dat}}
  {"2 D Intensity files" {.out}}
  {"chopper files" {.chp .par .dat}}
  {"crystal" {.crs .par .dat}}
  {"moderator (cws source)"  {.mod .cmo .src}}
  {"moderator (spss source)" {.mod .smo .imo .src}}
  {"moderator (lpss source)" {.mod .lmo .src}}
  {"powder sample" {.pow .par .dat}}
  {"sample s(q)" {.psq .par .dat}}
  {"sans sample" {.san .par .dat}}
  {"sample reflectometer" {.ref .dat}}
  {"sample singcryst" {.ssc .par .dat}}
  {"inelastic sample" {.ine .par .dat}}
  {"elastic isotropic sample" {.iso .par .dat}}
  {"polarizer sm" {.pol .par .dat}}
}


### neededModulesSET
###      (generated from AvailableSET)
### 0 name of module
### 1 selection of {active disabled simulated unused}
### 2 list of names of control variable lists for that module
###   (or "" if no control vars)
### 3 if not "", name of callback procedure
###   to set initial module constants
### 4 special procedure used to set control variables
###   (generateEntries is always possible)
### 5 if not "", name of help item for that module

proc makeModuleSets {} {
  global AvailableSET
  # 0 name of module categorie
  # 1 list of submodules; may be empty
  # 2 help item; may be a list if different submodules have different help
  set AvailableSET {
    {source {source_const_wave source_HMI source_ILL
      source_short_pulsed source_ESS source_IPNS source_ISIS source_SNS
      source_ESS_LPTS} source}
    {guide {guide bender} {guide bender}}
    {sm_ensemble {} sm_ensemble}
    {spacewindow {spacewindow spacewindow_multiple space}
      {spacewindow spacewindow_multiple}}
    {chopper {chopper_disc chopper_fermi_str chopper_fermi_cur} {chopper_disc chopper_fermi_str chopper_fermi_cur}}
    {velselect {} velselect}
    {collimator_soller {} collimator}
    {monochr_analyser {ma_flat ma_focus ma_focus_dat} monochr_analyser}
    {polariser {polariser_he3 polariser_sm pol_mirror} {polariser_he3 polariser_sm pol_mirror}}
    {flipper {flipper_coil flipper_gradient} {flipper_coil flipper_gradient}}
    {resonator_drabkin {} resonator_drabkin}
    {magnetic_field {precessionfield rotating_field} {precessionfield rotating_field}}
    {sample {sample_elasticisotr sample_inelast sample_powder
      sample_reflectom sample_sans sample_s_q sample_singcryst} {sample_elasticisotr sample_inelast
	sample_powder sample_reflectom sample_sans sample_s_q sample_singcryst}
    }
    {detector {} detector}
    {evaluation {eval_elast eval_inelast} {eval_elast eval_inelast}}
    {frame {} frame}
    {external_command}
    {writeout {} writeout}
    {visualise_data {
      visual
      mon1_time mon1_lambda mon1_energy mon1_y mon1_z mon1_divy mon1_divz 
      mon2_pos mon2_div mon2_kdiv mon2_tofwl mon2_wldiv mon2_y_divy mon2_z_divz
      monpol_time monpol_lambda monpol_y monpol_z
      monpol_divy monpol_divz monitorpol_pos
      } {visual monitor}}
  }

  # append special user module, if any
  catch {source [file join [pwd] GUI usermodule.tcl]}

  set aglob ""
  upvar #0 neededModulesSET a
  set a {}
  foreach line $AvailableSET {
    set s [lindex $line 1]
    set h [lindex $line 2]
    if {$s != ""} {
      foreach t $s m $h {
	if {$m == ""} {set m $h}
	append aglob " [set m ${t}ESET]"
	lappend a [list $t active $m "" "" $m]
      }
    } else {
      set n [lindex $line 0]
      append aglob " [set m ${n}ESET]"
      lappend a [list $n active $m "" "" $h]
    }
  }
  return $aglob
}
eval "global [makeModuleSets]"
rename makeModuleSets {}

###
### General positions & meaning in lists
###
### these lists should have name ending with (or at least including) ESET
### like singleDetectorESET
###
### 0 name of global variable (first part of name for type select)
### 1 type, one of {float int string longstring select radio
###                 filename editablefile browsefile browsedir
###                 parfilename pareditablefile parbrowsefile
###                 moneditablefile mon2editablefile}
###         browse indicates entries which are selectable by a file browser
###         par indicates a file which must reside in a
###            special default (parameter) directory
###         moneditablefile is a pareditablefile and a monitor output file of 1
###            dimensional data, where mon2editablefile is for 2 dimensional data
###
### 2 default value                 (meaningless for select)
### 3 comment list (item 0: label text,
###                 item 1: long description text (optional)
###                 item 2: callback procedure to show long descr. (optional)
###                 item 3: command option prefix string (optional))
###
### other entries depend on type
###
### for type {select}
### 4 list of pairs with {name_appendix default_bool}
###
### for types (browsefile browsedir editablefile parbrowsefile pareditablefile
###            moneditablefile mon2editablefile)
### 4 r for a readable file,
###   w for a valid filename
### 5 file extension, used to specify GUI-editable files
### 6 1 for mandatory
### 7 d for directory
###
### for types {filename string longstring}
### 4 like browsefile
### 5 dummy or empty
### 6 like browsefile
###
### for type {radio}
### 4 list of selectable items
### 5 list of corresponding keys (may be omitted)
###
### for types {float int}
### 4 min or 1 if no value 5, but a valid specification is necessary
### 5 max
###    for control variables additionally
### 6 not needed: iff 1, then an empty input is allowed
###
###
### An entry of type filename must specify an existing file or directory.
### The input may contain tilde (~) notation on unix systems.
### If a filename entry becomes checked, and the file or directory
### exists, it is replaced by a fully qualified filename. This helpss
### to avoid error situations, where a filename is used with a different
### default environment than xcontrol.
###

### Input parameters
###
set inputESET {
  {infilename parbrowsefile "" {"input file" "The data of all trajectories will be written to the 'output file' at the end (of the first part) of the simulation. These data can be used to start a second part the simulation by giving the name of this file as 'input file'." "" -f} r dat}

  {outfilename parbrowsefile "no_file" {"output file" "The data of all trajectories will be written to the 'output file' at the end (of the first part) of the simulation. These data can be used to start a second part the simulation by giving the name of this file as 'input file'." "" -F}}

  {defdirectory browsedir "" {
    "parameter\ndirectory" "This is the one and only directory for parameter files. All these files should reside in one directory, to make the reproduction of a simulation on other systems feasable."} w "" 1 d}

  {random_seed float 1 {
    "random seed" "random number generator initialization" "" -Z}}

  {random_gen radio ran3 {
    "random number\ngenerator" "Select a random number generator from the set of taus gfsr4 mt19937 ranlux ran3 (Default ran3)"}
    {ran3 taus gfsr4 mt19937 ranlux} {0 1 2 3 4}}

  {wei_min float 1.0e-25
    {"min. neutron\nweight" "minimal weight for tracing neutrons" "" -U} ge0}

  {gravity radio on
    {gravity "simulation includes gravity influence on neutrons or not" "" -G}
    {on off} {1 0}}
}

### Xcontrol defaults
###
set xcontrolDefaultsESET {
  {plotapp browsefile gnuplot {"plot application" "Application to be executed when the 'Ext. Plot file' title menu button is pressed. The application becomes called with a file name parameter."} r}
}


### moderator
###   mod file description

set m0 {
  {modtype radio -
    {"moderator\ntype" "Type of moderator (for pulsed sources).
multi-spectral :The effective flux distribution of a combination of a cold coupled and a thermal coupled moderator as achieved with an optimal beam extraction system."}
    {- "decoupled poisoned" "decoupled unpoisoned" coupled multi-spectral} {0 1 2 3 4}
  }
}

set m1 {
  {shape radio rectangular {shape "The moderator may have a rectangular or circular shape"} {rectangular circular} {S C}}
  {}
  {width float 0 {"moderator\ndiameter or\nwidth [cm]" "moderator width or diameter in cm"} ge0 "" 1}
  {height float 0 {
    "moderator\nheight [cm]" "moderator height in cm"} ge0 "" 1}
  {spaord int "" {"spatial\norder" "By this parameter it can be defined that one moderator is behind another. The higher the number the more it is in the background."} 0 32767}
  {}
  {cx float "" {"center of\nmoderator\nX [cm]" "The center of the source is usually (0.0,0.0,0.0).
In this case, neutrons coming from the center of the source without divergence pass the center of the window (if gravity is neglected).
Deviations of the moderator center from this position must be given here."}}
  {cy float "" {"center Y [cm]" "center of moderator y component (for further description see x component)"}}
  {cz float "" {"center Z [cm]" "center of moderator z component (for further description see x component)"}}
  {scale float ""
    {"total flux\nat moderator\n[n/(cm²s)]" "Flux on moderator surface into solid angle 2*pi integrated over wavelength [n/(cm²s)]\nMaxwellian or flux distribution from file are normalized to this value, (unless 'neutron current' is given)."}}
  {current float "" {"neutron\ncurrent [n/s]" "The current into the chosen solid angle is usually calculated as\ncurrent = total_flux * mod_area * solid_angle / (2*pi)\nand thus need not be given.\nIf moderator area or solid angle are chosen to be zero, it can be useful to give a value for the current (into the solid angle). Otherwise the spectrum is normalized to have an integral of 1.\nWarning: If a current value is given, the 'total flux' value is ignored!"}}
}

set m2 {
  {}
  {wfile pareditablefile "" {"user wavelength\ndist. file" "Name of the file that contains the wavelength distribution function M(lambda) for the moderator used. units:
\tCW: [Ang], [n/(cm² s str Ang)]
\tSS: [Ang], M(lambda) * F(t) must have the unit [n/(cm² s str Ang)]
(cf. user time dist. file)"}}
  {temp float 0 {"moderator\ntemperature [K]" "the temperature is only needed and used, if no wavelength dist. file is given"} ge0}
  {color int "" {colour "The trajectories can be marked by a so-called 'colour' to identify, which moderator they come from."} 0 32767}
}

set m3 {
  {}
  {wtfile pareditablefile "" {"user wavelength\ntime dist. file" "Name of the file that contains the wavelength-time distribution function F(lambda,t) for the moderator used. Unit: [n/(cm² s str Ang)]"}}
  {tau1 float ""
    {"tau_1 [us]" "First time constant of the pulse in microseconds (this is thought to be the smaller one of the two time constants).\nThe time constants are only used, if no time distribution file is given. See help file for details."} ge0}
  {tau2 float ""
    {"tau_2 [us]" "Second time constant of the pulse in microseconds (this is thought to be the larger of the two time constants). In this case it describes the decay of the pulse (for t >> tau_1).\nThe time constants are only used, if no time distribution file is given. See help file for details."} ge0}
  {}
  {tfile pareditablefile "" {"user time\ndist. file" "Name of the file that contains the time distribution function F(t) for the moderator used.
  units: [ms], M(lambda) * F(t) must have the unit [n/(cm² s str Ang)]
  (cf. user wavelength dist. file)"}}
}

set Mod1 {}
set Mod2 {}
foreach e [concat $m1 $m2] {
  set n [lindex $e 0]
  if {$n == "" || [lindex $e 1] == "header"} {
    set i1 [set i2 $e]
  } else {
    set i1 [lreplace $e 0 0 ${n}1]
    set i2 [lreplace $e 0 0 ${n}2]
  }
  lappend Mod1 $i1
  lappend Mod2 $i2
}

### cws source module description

set cmoESET [concat {
  {"Moderator 1" header}
} $Mod1 {
  {"Moderator 2" header}
  {usemod2 radio unused {"second moderator"} {used unused} {1 0}}
} $Mod2 ]

### ISIS variant

set imoESET {
  {Moderator header}
  {width float 0 {"moderator\ndiameter or\nwidth [cm]" "moderator width or diameter in cm"} ge0 "" 1}
  {height float 0 {"moderator\nheight [cm]" "moderator height in cm"} ge0 "" 1}
  {}
  {cx float "" {"center of\nmoderator\nX [cm]" "The center of the source is usually (0.0,0.0,0.0).
In this case, neutrons coming from the center of the source without divergence pass the center of the window (if gravity is neglected).
Deviations of the moderator center from this position must be given here."}}
  {cy float "" {"center Y [cm]" "center of moderator y component (for further description see x component)"}}
  {cz float "" {"center Z [cm]" "center of moderator z component (for further description see x component)"}}
  {}
  {tstat radio TS1 {"target\nstation"} {TS1 TS2} {1 2}}
  {}
  {wtfile pareditablefile "" {"user wavelength\ntime dist. file" "Name of the file that contains the wavelength-time distribution function F(lambda,t) for the moderator used. Unit: [n/(cm² s str Ang)]"}}
}

### pulsed sources

set Mod1 {}
set Mod2 {}
foreach e [concat $m0 $m1 $m2 $m3] {
  set n [lindex $e 0]
  if {$n == "" || [lindex $e 1] == "header"} {
    set i1 [set i2 $e]
  } else {
    set i1 [lreplace $e 0 0 ${n}1]
    set i2 [lreplace $e 0 0 ${n}2]
  }
  lappend Mod1 $i1
  lappend Mod2 $i2
}

### spss source module description
set smoESET [concat {
  {"Moderator 1" header}
} $Mod1 {
  {"Moderator 2" header}
  {usemod2 radio unused {"second moderator"} {used unused} {1 0}}
} $Mod2 ]

### lpss source module description
set lmoESET $smoESET

# all pulsed source moderator descriptions need a big scrollable edit frame
set BigFramesmo 1
set BigFramelmo 1

################################################################################
### VITESS modules
### Source parameters

set smASET {
  {"Restriction of sampling trajectories" header}
  {number_of_neutrons float 1000000 {
    "number of\ntrajectories" "" "" n} ge0 "" 1}
  {}
  {min_wavelength float 1.0 {"min. wave-\nlength [A]" "" "" m} ge0 "" 1}
  {min_time float "" {"min. time [ms]" "minimal time in ms of time window at moderator" "" t}}
  {phi float 0.5 {
    "max. divergence\nx <-> y [deg]"
    "max divergence phi [deg] (half of angular spread x-y-plane)"
    "" y} le90}
  {}
  {max_wavelength float 10 {"max. wave-\nlength [A]" "" "" M} gt0 "" 1}
  {max_time float "" {"max. time [ms]" "maximal time in ms of time window at moderator" "" T}}
  {theta float 0.5 {
    "max. divergence\nx <-> z [deg]"
    "maximal divergence theta [deg] (half of angular spread x-z-plane)"
    "" z} le90}
  {dirdet radio "by divergence" {"direction\ndefined" "The distribution of flight directions can be given by the maximal divergence from the straight flight direction (items 'max. divergence').
  Alternatively, the directions can defined by MC choices of positions where they pass the window (see 'Propagation') in addition to the starting point on the moderator surface. 
  In this case the given values in 'max. divergence ...' are ignored. Virtual window means that the neutrons are NOT propagated to the window, but remain on the moderator surface instead." "" d}
    {"by divergence" "by window" "by virtual window"} {0 1 2}}
  {}
}

set smisisASET {
  {"Restriction of sampling trajectories" header}
  {number_of_neutrons float 1000000 {
    "number of\ntrajectories" "" "" n} ge0 "" 1}
  {}
  {min_wavelength float 1.0 {"min. wave-\nlength [A]" "" "" m} ge0 "" 1}
  {max_wavelength float 10 {"max. wave-\nlength [A]" "" "" M} gt0 "" 1}
  {" " header}
}

set traceASET {
  {poldeg float 0
    {"degree of pola-\nrization [%]" "percentage of polarisation" "" P} 0 100}
  {}
  {polx float 1
    {"polarisation X\ndirection" "X-component of the polarisation direction" "" X}}
  {poly float 0
    {Y "Y-component of the polarisation direction" "" Y}}
  {polz float 0
    {Z "Z-component of the polarisation direction" "" V}}
  {}
}

### Source parameters
###
set cwsASET {
  {Propagation header}
  {dist_mod_prop float 200 {
    "distance to\nwindow [cm]"
    "distance between moderator and propagation window in cm.
    If the moderator is not positioned at the origin (0.0,0.0,0.0), it is the distance from the origin." "" D} ge0 "" 1}
  {prop_width float 10 {
    "window\nwidth [cm]"
    "width of propagation window in cm" "" w} gt0 "" 1}
  {prop_height float 10 {
    "window\nheight [cm]"
    "height of propagation window in cm" "" h} gt0 "" 1}
  {decl float 0 {"declination\n[deg]"
    "declination of the aperture center (= beam direction) to the normal of the moderator surface (in the horizontal plane)" "" i}}
  {"Special simulation parameters" header}
  {timemeas float 0
    {"time of\nmeasurement [s]" "not necessary: the number of neutrons for the given time range is calculated in each module, if the time is not zero." "" A} ge0}
  {deswl float "" {"desired\nwavelength [A]" "not necessary: (average) wavelength (at the sample) to be used in the measurement - not necessary, only needed to write optimal chopper phases to 'instrument.inf'" "" W}}
  {}
  {trace radio no {"kind of\nraytracing" "It is supposed that a first run has delivered the 'raytracing file' that contains all trajectories of interest.
For each trajectory found in the 'raytracing file' a data file is generated and each module writes information to this file. To do that the whole simulation is repeated.
Option 'only trace trajectories'
Only those trajectories are started in the second run that are found in the 'raytracing file'.
(This yields identical results at (or after) the site where the trajectories of interest were determined, only if there are no MC choices in the devices between source and the site of interest, i.e. no sample, no monochromator/analyser, no sm_ensemble, no bender with transmission between channels." "" k} {no "write trace files" "only trace trajectories"} {0 1 2}}
  {utrcfunction editablefile ""
    {"raytracing file" "Name of the file that contains the ID of the trajectories for tracing." "" r}}
}

### source
###   CWS continuous wavelength sources

set li {"moderator\ndescription file" "Name of the file containing the description of the source and one or two moderators. Existing files are in FILES/moderators" "" a}

foreach s {const_wave HMI ILL} \
        m {ReactorCold HmiMS IllColdSrcCold} {
  set al [list modfile pareditablefile $m.mod $li w cmo 1]
  set source_${s}ESET [concat [list $al] $smASET $traceASET $cwsASET]
  proc ${s}CheckErr {{app _}} {source_cwsCheckErr $app}
}

proc source_cwsCheckErr {{app _}} {
  foreach l {dist_mod_prop dirdet}  {
    upvar #0 $l$app $l
  }
  if {$dist_mod_prop == 0 && $dirdet == "by window"} {
    showText "!Direction can only be defined by window, if distance moderator to window > 0"
    return 1
  }
  return [checkMiMaErr min_wavelength max_wavelength wavelength $app]
}

### source
###   SPSS short pulsed spallation sources

proc sore {f s} {
  set f [list [list freq float $f {"pulse repetition\nrate [Hz]" "" "" R} 1]]
  set s [list [list name radio $s {"analytical flux\ncalculation for" "flux can be calculated analytically for ESS and SNS\ntemperature, tau-values and dist. files ignored in this case" "" N} {- ESS SNS} {- ESS SNS}]]
  return [concat $f $s]
}

foreach s {short_pulsed ESS IPNS SNS} \
        m {SPTScold EssSPThermDec IpnsSPThermPois SnsColdCpld} \
        fr {50 50 50 60} \
        sps {- ESS - SNS} {
  set al [list modfile pareditablefile $m.mod $li w smo 1]
  set fl [sore $fr $sps]
  set source_${s}ESET [concat $fl [list $al] $smASET $traceASET $cwsASET]
  proc ${s}CheckErr {{app _}} {return [source_cwsCheckErr $app]}
}

proc sore {f} {
  return [list [list freq float $f {"pulse repetition\nrate [Hz]" "" "" R} 1]]
}

set al [list modfile pareditablefile IsisTS1hydrogen.mod $li w imo 1]
set fl [sore 50]
set source_ISISESET [concat $fl [list $al] $smisisASET $traceASET $cwsASET]
proc ISISCheckErr {{app _}} {return [source_cwsCheckErr $app]}


### source
###   LPSS long pulsed spallation sources

set al [list modfile pareditablefile EssLPMs.mod $li w lmo 1]
set source_ESS_LPTSESET [concat {
  {name radio ESS {"name of source" "" "" N} {- ESS SNS} {- ESS SNS}}
  {freq float 16.667 {"pulse repetition\nrate [Hz]" "" "" R} 1}
  {plen float 2.0 {"proton pulse\nlength [ms]" "time dependence of neutron flux
     \tt < p:  1/s*[1-exp(-t/beta)]
     \tt >= p: 1/s*[1-exp(-p/beta)]*[-(t-p)/beta]" "" p} 1}
} [list $al] $smASET $traceASET $cwsASET]

proc source_ESS_LPTSCheckErr {{app _}} {
  foreach l {tau1 tau2 name}  {
    upvar #0 $l$app $l
  }
  if {$name == "" && ($tau1 == "" || $tau2 == "")} {
    showText "!Please specify tau1 and tau2 or select a known source"
    return 1
  }
  return [source_cwsCheckErr $app]
}

### Detector
###
set detectorESET {
  {"Detector geometry" header}
  {geom radio flat {geometry
    "The geometry parameter specifies the geometry of the detector. There are rectangular or cylindrical detectors." "" G}
    {flat cylindrical} {cub cyl}}
  {}
  {hei float 10 {
    "height [cm]" "Height of the detector in cm." "" h}
    gt0 "" 1}
  {wid float 10 {
    "width [cm]" "Full width of a flat detector in cm.In case of a cylindrical detector it is the length of the cylinder arch under consideration." "" w}
    gt0 "" 1}
  {thick float 0.2 {
    "thickness [cm]" "Thickness of the detecting material in cm." "" t}
    gt0 "" 1}
  {nrow int 1 {
    "number\nof rows" "Number of rows partitioning the detector height." "" r} 1 10000 1}
  {ncol int 1 {
    "number\nof columns" "Number of columns of the detector." "" c} 1 10000 1}
  {eff float 0.95 {
    efficiency "Efficiency of the detector, range: 0<Efficiency<0.99999." "" e} gt0 1 1}
  {phi float 0 {
    "phi [deg]" "Angle phi [0;360 deg] of the middle of the detector, i.e. the angle between the projection of the position vector to the yz-plane and the +y-axis. For cylindrical geometry phi must be 0 or 180!" "" P} 0 360 1}
  {theta float 0 {
    "theta [deg]" "Angle theta [0;180 deg] of the middle of the detector. Theta is defined as the angle between the position vector (pointing from the origin to the detector centre) and the +x-axis." "" T} 0 180 1}
  {dist float 100 {
    "distance [cm]" "Distance of the centre of the detector surface to the origin (0,0,0) in cm. In case of a cylindrical detector this is the cylinder radius." "" D} ge0 "" 1}
  {repr int 1 {
    repetition "The neutron repetition specifies the number of neutron data sets generated for each scattered neutron." "" A} 1}
  {use radio normal {
    usage "If 'monitor only' is selected, use detector geometry only as a monitor, i.e. the weight and flight direction of the trajectory are unchanged; otherwise thickness, efficiency and wavelength are used to calculate a count rate that can be expected in experiments." "" M}
    {normal "monitor only"} {0 1}}
  {grid radio on {
    "detector grid" "If the detector grid is switched off, the exact neutron position is written to the output file." "" g}
    {on off} {1 0}}
  {det_tof radio calc {
    "TOF option" "no: flight path inside detector is set to zero\ncalc: length and TOF calculated from thickness" "" o}
    {no calc} {0 1}}
}

proc detectorCheckErr {{app _}} {
  foreach l {geom wid phi}  {
    upvar #0 $l$app $l
  }
  if {$geom == "flat"} {
    if {$wid == ""} {
      showText "!Please specify the full width for a flat detector"
      return 1
    }
  } else {				# $geom == "cylindrical"
    if {$phi != 0 && $phi != 180} {
      showText "!Please specify phi as either 0 or 180 for a cylindrical geometry"
      return 1
    }
  }
  return 0
}

### External Command
###
set external_commandESET {
  {extern_com browsefile "" {"external\nexe-file" "Full filename path of the executable program."} r "" 1}
  {Options header}
  {extern_shortopt longstring "" {"option\nstring" "This option string is passed as it is to the external command."}}
  {extern_optfile pareditablefile "" {"option\nfile" "File with options for the external command. This file may contain several lines, which are concatenated to a blank separated string."}}
}


### Writeout
###
set writeoutESET {
  {fname pareditablefile noutascii.dat {
    "ASCII\nfile name" "Specifies the name of the ASCII file." "" A} w "" 1}
  {outform radio float {"data format" "format of double values in writeout file" "" F} {exp float} {0 1}}
}

### Frame
###
set frameESET {
  {Transformation header}
  {seq radio RTM {sequence "sequence of Rotation, Translation, and Mirroring" "" S}
    {RTM RMT TRM TMR MTR MRT} {1 2 3 4 5 6}}
  {Rotation header}
  {rotz float 0 {"rot. angle [deg]\naround z axis" "rotation angle around Z FIRST rotation [deg]\nNOTE: 90deg means X+  rotated on Y+" "" H} 1}
  {roty float 0 {"rot. angle [deg]\naround y axis" "rotation angle around Y SECOND rotation [deg]\nNOTE: 90deg means Z+ rotated on X+" "" V} 1}
  {rotx float 0 {"rot. angle [deg]\naround x axis" "rotation angle around X (beam axis) THIRD rotation [deg]\nNOTE: 90deg means Y+  rotated on Z+" "" A} 1}
  {Translation header}
  {tx float 0 {"x [cm]" "x component of translation vector [cm]" "" x} 1}
  {ty float 0 {"y [cm]" "y component of translation vector [cm]" "" y} 1}
  {tz float 0 {"z [cm]" "z component of translation vector [cm]" "" z} 1}
  {Mirror header}
  {mx radio no {"mirror at\nyz plane" "mirror x axis (plane yz)" "" i} {no yes} {0 1}}
  {my radio no {"mirror at\nxz plane" "mirror y axis (plane xz)" "" j} {no yes} {0 1}}
  {mz radio no {"mirror at\nxy plane" "mirror z axis (plane xy)" "" k} {no yes} {0 1}}
}

### Spacewindow
###
set winAdd {
  {"Outer Material" header}
  {thick float 0.0 {"thickness of\nmaterial [cm]" "Thickness of material, which was used for collimator." "" t} ge0}
  {mat radio "ideal absorber" {material "Choose material, which was used to produce the collimator" "" c}
    {"from file" gadolinium cadmium Bor10 Eu Silicon "ideal absorber"}
    {0 1 2 3 4 5 6}}
  {matfile browsefile "" {"material\ndescription file" "File which characterizes the transmission of the outer material of the collimator." "" C}}
  {"Inner Material" header}
  {imatfile browsefile "" {"material\ndescription file" "File which characterizes the transmission of the inner material of the collimator." "" m}}
  {imathick float 0 {"thickness of\nmaterial [cm]" "Thickness of inner material, which was used for the collimator." "" T} ge0}
}

set a {
  {dist_orig_window float 0 {
    "distance orig.\n  <-> win. [cm]"
    "distance from origin to window when projecting along the x axis"
    "" l} ge0 "" 1}
  {circ radio circular {"window shape" "" "" R} {circular rectangular} {1 0}}
  {"circular window coordinates" header}
  {radi float 10 {radius "radius of circular window" "" r} gt0}
  {centy float 0 {"center y" "" "" y} }
  {centz float 0 {"center z" "" "" z} }
  {"rectangular window coordinates" header}
  {min_z float "" {
    "min. z [cm]" "minimal z value [cm]" "" h}}
  {max_z float "" {
    "max. z [cm]" "maximal z value [cm]" "" H}}
  {}
  {min_y float "" {
    "min. y [cm]" "minimal y value [cm]" "" w}}
  {max_y float "" {
    "max. y [cm]" "maximul y value [cm]" "" W}}
  {useasbstop radio no {
    "used as\nbeamstop" "The spacewindow module can be used as beamstop. If so, the trajectory is lost." "" S}
    {no yes} {0 1}
  }
  {oldframe radio no {
    "use previous\nframe" "yes: the frame of the previous module is used (default for beamstop)\nno : x-component of frame is shifted to the window plane (default for window)" "" F}
    {no yes} {0 1}
  }
}

set spacewindowESET [concat $a $winAdd]


proc windowCheckErr {{app _}} {
  foreach l {circ radi centy centz min_z max_z min_y max_y} {
    upvar #0 $l$app $l
  }
  if {$circ == "yes"} {
    if {$radi == "" || $centy == "" || $centz== ""} {
      showText "!Please specify radius and center parameters"
      return 1
    }
  } else {
    if {$min_z == "" || $max_z == "" || $min_y == "" || $max_y == ""} {
      showText "!Please specify all min/max values for y and z"
      return 1
    } elseif {$min_y > $max_y} {
      showText "!Please input min_y <= max_y"
      return 1
    } elseif {$min_z > $max_z} {
      showText "!Please input min_z <= max_z"
      return 1
    }
  }
  return 0
}

### Spacewindow Multiple
###
set a {
  {colfile pareditablefile "" {
    "collimator\nfile" "File which contains the circular hole(s) data. Each line must contain a triple of data describing a hole : y,z,radius. The number of holes for each collimator must be less than 100. (see help)"
    "" I} r dat}
  {dist float 0 {
    "distance orig\n<->win [cm]" "Distance to window along x-direction  [cm]" "" D} ge0}
  {rad float 100 {
    "Outer\nradius [cm]" "Outer radius of the circular plate (multiaperture collimators) [cm]" "" r} gt0}
}

set spacewindow_multipleESET [concat $a $winAdd]


### Space
set spaceESET {
  {dist float "" {"distance [cm]" "" "" d} gt0}
}

### Guide
###
set guideESET {
  {"Shape and size of guide" header}
  {keyshape_y radio constant {"horizontal\nshape" "shape of the guide in x-y-plane" "" Y}
    {constant linear curved parabolic elliptic} {0 1 2 3 4}}
  {keyshape_z radio constant {"vertical\nshape" "shape of the guide in x-z-plane" "" Z}
    {constant linear parabolic elliptic} {0 1 3 4}}
  {}
  {enter_width float 10 {
    "entrance\nwidth [cm]"
    "entrance of guide: width in cm (center of entrance window = origin)"  "" w} gt0 "" 1}
  {enter_height float 10 {
    "entrance\nheight [cm]"
    "entrance of guide: height in cm (center of entrance window = origin)" "" h} gt0 "" 1}
  {}
  {exit_width float 10 {
    "exit\nwidth [cm]"
    "exit of guide: width in cm (center of exit window = new origin)"  "" W} gt0 "" 1}
  {exit_height float 10 {
    "exit\nheight [cm]"
    "exit of guide: height in cm (center of exit window = new origin)" "" H} gt0 "" 1}
  {"Guide characteristics" header}
  {len_guide_piece float "" {
    "piece\nlength [cm]" "length of a guide piece [cm]" "" p} ge0 "" 1}
  {number_pieces int 1 {
    "number of\npieces" "number of guide pieces" "" N} gt0 "" 1}
  {rad_curve float 0 {
    "curvature\n(radius) [m]"
    "radius of curvature [m] (0 means no curvature, > 0 to the left,\n < 0 to the right)" "" R}}
  {"Reflectivity files" header}
  {lrefl_filename pareditablefile mirr1a.dat
    {"left plane" "Reflectivity file for left plane (where y>0)" "" i} r dat 1}
  {rrefl_filename pareditablefile mirr1a.dat
    {"right plane" "Reflectivity file for right plane (where y<0)" "" I} r dat}
  {tbrefl_filename pareditablefile mirr1a.dat
    {"top plane" "Reflectivity file for top (and bottom) plane" "" j} r dat 1}
  {brefl_filename pareditablefile ""
    {"bottom plane" "Reflectivity file for bottom plane" "" J} r dat}
  {"Bender option" header}
  {num_channels int "" {
    "number of\nchannels" "number of channels (lying in the x-z-plane)" "" b} ge0}
  {spacer_width float "" {
    "substrate\nwidth [cm]" "thickness of material dividing the guide/bender into channels" "" s} ge0}
}


set specoptAdd {
  {"Special options" header}
  {waviness float 0
    {"surface\nwaviness [deg]" "This parameter controls the simulation of surface waviness. This value is the maximal angle of deviation of the surface normal from the ideal normal." "" r}}
  {h_focus_pnt float 0 {
    "hor. focus dist.\nof ellipse [cm]"  "only for elliptic shape: distance between guide exit and focus point of ellipse for horizontal focussing"  "" f} ge0}
  {v_focus_pnt float 0 {
    "vert. focus dist.\nof ellipse [cm]" "only for elliptic shape: distance between guide exit and focus point of ellipse for vertical focussing"  "" F} ge0}
  {}
  {keyabut radio no {"abutment\nloss"
    "Neutrons that hit the surface close to one of the ends of the guide/bender (or a guide segment) are rejected." "" a}
    {yes no} {1 0}}
}

set guideESET [concat $guideESET $specoptAdd]

proc guideCheckErr {{app _}} {
  set enwi [entryVal enter_width $app]
  set exwi [entryVal exit_width $app]
  set cur  [entryVal rad_curve $app]
  if {$enwi != $exwi && $cur != 0} {
    showText "Curvature and horizontal (de-)compression is not supported"
    return 1
  }
  return 0
}

### Bender
###
### special options: h H s l R; i m k; I M K; u A; g c; z w; C T O; r a y p V t; o

set BigFramebender 1

set benderESET {
  {visdev radio display {visualisation\ndevice "" "" o} {display file display+file} {1 2 3}}
  {"Bender geometry characteristic" header}
  {enter_height float 10 {
    "entrance\nheight [cm]"
    "entrance of guide: height in cm (center of entrance window = origin)" "" h} gt0 "" 1}
  {exit_height float 10 {
    "exit\nheight [cm]"
    "exit of guide: height in cm (center of exit window = new origin)" "" H} gt0 "" 1}
  {swidth float 0 {"substrate\nwidth [cm]"
    "thickness of material dividing the guide/bender into channels" "" s} ge0 "" 1}
  {len_guide float 100 {
    "length [cm]" "length of a guide [cm]. Specify either length or filename." "" l} gt0 "" 1}
  {curvrad float 0 {"radius of\ncurvature [cm]"
    " radius of curvature of base circle-axis of bender(if zero - straight line)" "" R} ge0 "" 1}

  {"Reflectivity files for spin up" header}
  {lrefl_filename pareditablefile mirr0.dat
    {"left plane" "Reflectivity file for left plane (where y>0) and spin is up" "" i} r dat 1}
  {rrefl_filename pareditablefile mirr2linear.dat
    {"right plane" "Reflectivity file for right plane (where y<0) and spin is up" "" m} r dat 1}
  {tbrefl_filename pareditablefile mirr0.dat
    {"top/bot. plane" "Reflectivity file for top and bottom plane and spin is up" "" k} r dat 1}

  {"Reflectivity files for spin down" header}
  {dlrefl_filename pareditablefile mirr0.dat
    {"left plane" "Reflectivity file for left plane (where y>0) and spin is down" "" I} r dat 1}
  {drrefl_filename pareditablefile mirr0.dat
    {"right plane" "Reflectivity file for right plane (where y<0) and spin is down" "" M} r dat 1}
  {dtbrefl_filename pareditablefile mirr0.dat
    {"top/bot. plane" "Reflectivity file for top and bottom plane and spin is down" "" K} r dat 1}

  {"Geometrical description of bender" header}
  {sfile pareditablefile "" {"surface\nfile" " file which describes the bender geometry" "" u} r}
  {ifile parbrowsefile "" {"information\nfile" " file which contains some information about the bender geometry" "" A} r}

  {"Special option" header}
  {uref radio yes {"transmitted\nneutrons"
    "no: ideal absorption between bender channels,\nyes: unreflected neutrons pass in the next bender channel" "" g}
    {yes no} {1 0}}
  {amat radio Vacuum {"Absorption\nmaterial"
    "Attenuation of neutrons flux in the given material inside channels
of bender.\n\"from file\" means data are read from a file specified as \"transmission file\"" "" c}
    {"from file" Gadolinium Cadmium Bor10 Eu Silicon Vacuum} {0 1 2 3 4 5 6}}
  {}
  {amatl radio Gadolinium {"left side\nmaterial"
    "Absorption material in the left side of channel. Unreflected neutrons pass in the next channel via absorbtion material in the inner(left) side of channel.\n\"from file\" means data are read from a file specified as \"left side file\"" "" z}
    {"from file" Gadolinium Cadmium Bor10 Eu Silicon Vacuum} {0 1 2 3 4 5 6}}
  {amatr radio Gadolinium {"right side\nmaterial"
    "Absorption material in the right side of channel. Unreflected neutrons pass in the next channel via absorbtion material in the outer(right) side of channel.\n\"from file\" means data are read from that file specified as \"right side file\"" "" w}
    {"from file" Gadolinium Cadmium Bor10 Eu Silicon Vacuum} {0 1 2 3 4 5 6}}
  {}
  {mfile pareditablefile "" {"transmission\nfile" "File which characterizes the transmission of bender channel material.\nInput this file name, if \"Absorption material\" has been set to \"from file\"" "" C} r}
  {mfilel pareditablefile "" {"left side\nfile" "File which characterizes the transmission of material in the left side of the bender.\nInput this file name, if \"left side material\" has been set to \"from file\"" "" T} r}
  {mfiler pareditablefile "" {"right side\nfile" "File which characterizes the transmission of material in the right side of the bender.\nInput this file name, if \"left side material\" has been set to \"from file\"" "" O} r}
  {}
  {surwav float 0 {"surface\nwaviness [deg]"
    "This parameter controls the simulation of surface waviness. This value is the maximal angle of deviation of the surface normal from the ideal normal." "" r} ge0 "" 1}
  {abut float 0 {"abutment\nloss length"
    "Neutrons that hit the surface close to one of the ends of the guide/bender (or a guide segment) are rejected." "" a} ge0 "" 1}
  {visu radio yes {visualisation "" "" y} {yes no} {1 0}}
  {pola radio yes {polarisation
    "yes: split into spin-down and spin-up reflectivity\nno: spin-up reflectivity for all neutrons" "" p}
    {yes no} {1 0}}
  {spiqua radio OX {"neutron\nspin axis" "axis for spin quantisation" "" V} {OX OY OZ} {0 1 2}}
  {geotest radio yes {"geometry test" "activate/deactivates geometry test" "" t} {no yes} {0 1}}
}

proc checkOFile {a b ts app} {
  if {[entryVal $a $app] != "from file"} {return 0}
  set f [entryVal $b $app]
  global defdirectory_
  if {$f != "" && [file exists [file join $defdirectory_ $f]]} {return 0}
  showText "!use an existing transmission file$ts"
  return 1
}

proc benderCheckErr  {{app _}} {
  set rc 0
  if [checkOFile amat mfile "" $app] {set rc 1}
  if [checkOFile amatl mfilel " for the left side" $app] {set rc 1}
  if [checkOFile amatr mfilel " for the right side" $app] {set rc 1}
  return $rc
}


### velselect
###
set velselectESET {
  {length float 30 {
    "length of\nvelselect [cm]" "length of velocity selector [cm]" "" l} gt0}
  {rotations float 500 {
    "rotations\nper sec." "number of rotations per second" "" s} 1}
  {channels int 72 {
    "number of\nchannels" "number of velocity selector channels" "" w} ge1}
  {curvature float 45 {
    "curvature [deg]" "curvature of velocity selector channels " "" c} 1}
  {radius float 20 {
    "radius [cm]" "radius of the velocity selector front"
    "" r} gt0}
  {distance float 17 {
    "vert. distance\naxle-orig. [cm]" "distance (along z-axis) between axle of velocity selector and the origin (generally the center of the neutron guide end from the last module),
preferably <= radius - 0.5*height of guide" "" o} 1}
  {spacew float 0 {
    "spacer\nwidth [cm]" "width of a spacer which separates the channels of the velocity selector" "" d}
    0 1000}
}

proc velselectCheckErr {{app _}} {
  set d [entryVal distance $app]
  set r [entryVal radius $app]
  if {$d > $r} {
    showText "!the distance between axle and origin should be <= radius"
    return 1
  }
  return 0
}

### chopper_disc
###
set chopper_discESET {
  {rounds float 6000 {"rounds / min." "rounds per minute" "" s} 1}
  {offset float 0 {"Offset [deg]" "initial chopper offset at t=0" "" o} 1}
  {absorption radio ideal {
    absorption
    "ideal: perfect chopper absorption\nGd: imperfect chopper absorption by gadolinium\nBor: imperfect chopper absorption by bor-10" "" g}
    {ideal Gd Bor} {0 1 2}}
  {time_to_zero radio no {
    "set zero time"
    "yes: chopper defines zero time\nno: zero time is defined in source" "" z}
    {yes no} {1 0}}
  {pass_outside radio yes {
    "treat neutrons\npassing by"
    "yes: neutrons passing outside the chopper disc are treated\nno: neutrons passing outside are removed" "" p}
    {yes no} {1 0}}
  {wnd_colour radio no {
    "set colour"
    "yes: colour of neutrons will be defined by window that they are passing\nno: color set before is kept" "" c}
    {yes no} {1 0}}
  {chop_file pareditablefile chop_105.dat {
    "chopper file"
    "file with chopper data\n(position, radius, number of windows, window opening, left and right angular deviation of window)" "" C} r chp 1}
  {dist float 0 {
    "distance to\nprev. module [cm]" "distance between chopper and origin generated by the antecedent module along x-axis" "" l} ge0 "" 1}
  {equwnd int 1 {"No of equ.\nwindows" "number of equivalent windows on chopper disc used to generate pulses" "" n} 1}
}

### chopper
###   chp file description
set chpESET {
  {nwindows int 1 {"number\nof windows"} 1 4 1}
  {radius float "" {"radius [cm]" "radius of chopper"} gt0 "" 1}
  {}
  {distance float "" {
    "vert. position\n of axle [cm]" "z component of the chopper centre in the coordinate system defined by the previous module (usually the centre of the beamline)"}}
  {hdistance float "" {
    "horiz. position\n of axle [cm]" "y component of the chopper centre in the coordinate system defined by the previous module (usually the centre of the beamline)"}}
  {"first window" header}
  {winpos0 float "" {
    "window\nposition [deg]" "angular position of window centre"}}
  {winheight0 float "" {
    "window\nheight [cm]" "window height from edge of chopper disk to bottom of window "}}
  {width0 float "" {
    "window\nwidth [deg]" "angular opening of chopper window"}}
  {ldeviation0 float "" {
    "left side\ndeviation [deg]" "Angular deviation of left window side (see graph in help manual), positive value indicates that window widens"}}
  {rdeviation0 float "" {
    "right side\ndeviation [deg]" "Angular deviation of right window side (see graph in help manual), positive value indicates that window widens"}}
  {"2nd window (if at least 2 windows)" header}
  {winpos1 float "" {
    "window\nposition [deg]" "angular position of window centre"}}
  {winheight1 float "" {
    "window\nheight [cm]" "window height from edge of chopper disk to bottom of window "}}
  {width1 float "" {
    "window\nwidth [deg]" "angular opening of chopper window"}}
  {ldeviation1 float "" {
    "left side\ndeviation [deg]" "Angular deviation of left window side (see graph in help manual), positive value indicates that window widens"}}
  {rdeviation1 float "" {
    "right side\ndeviation [deg]" "Angular deviation of right window side (see graph in help manual), positive value indicates that window widens"}}
  {"3rd window (only if 3 windows)" header}
  {winpos2 float "" {
    "window\nposition [deg]" "angular position of window centre"}}
  {winheight2 float "" {
    "window\nheight [cm]" "window height from edge of chopper disk to bottom of window "}}
  {width2 float "" {
    "window\nwidth [deg]" "angular opening of chopper window"}}
  {ldeviation2 float "" {
    "left side\ndeviation [deg]" "Angular deviation of left window side (see graph in help manual), positive value indicates that window widens"}}
  {rdeviation2 float "" {
    "right side\ndeviation [deg]" "Angular deviation of right window side (see graph in help manual), positive value indicates that window widens"}}
  {"4th window (only if 4 windows)" header}
  {winpos3 float "" {
    "window\nposition [deg]" "angular position of window centre"}}
  {winheight3 float "" {
    "window\nheight [cm]" "window height from edge of chopper disk to bottom of window "}}
  {width3 float "" {
    "window\nwidth [deg]" "angular opening of chopper window"}}
  {ldeviation3 float "" {
    "left side\ndeviation [deg]" "Angular deviation of left window side (see graph in help manual), positive value indicates that window widens"}}
  {rdeviation3 float "" {
    "right side\ndeviation [deg]" "Angular deviation of right window side (see graph in help manual), positive value indicates that window widens"}}
}

proc chpCheckErr {{app _}} {
  set err 0
  foreach l {nwindows radius} {
    upvar #0 $l$app $l
  }
  for {set i 0} {$i < 3} {incr i} {
    foreach l {winpos winheight width ldeviation rdeviation} {
      upvar #0 $l$i$app $l
    }
    if {$i < $nwindows} {
      if {$winpos == "" || $winheight == "" || $width == "" \
	      || $ldeviation == "" || $rdeviation == ""} {
	showText "!Please specify all entries for window  [expr $i + 1]"
	set err 1
      } elseif {"" == $winheight < 0 || $winheight > $radius} {
	if {$radius == ""} {set rr radius} else {set rr $radius}
	showText "!0 <= winheigth$i <= $rr"
	set err 1
      }
    } else {
      if {$winpos != "" || $winheight != "" || $width != "" \
	      || $ldeviation != "" || $rdeviation != ""} {
	showText "!Did you want [expr $i + 1] windows? Then specify number of windows accordingly."
	set err 1
      }
    }
  }
  return $err
}

### chopper
###
set chop1Add {
  {x float 10 {"position\nX [cm]" "center position x of the Fermi chopper" "" X}}
  {y float 0 {"position\nY [cm]" "center position y of the Fermi chopper" "" Y}}
  {z float 0 {"position\nZ [cm]" "center position z of the Fermi chopper" "" V}}
  {a float 5 {"height [cm]" "height of the Fermi chopper" "" a} gt0}
  {b float 4 {"width [cm]" "width of the Fermi chopper" "" b} gt0}
  {c float 3 {"channel\nlength [cm]" "channel length of the Fermi chopper (not active for channel shape option 'ideal')" "" c} gt0}
  {chans int 20 {"number of\nchannels" "number of straight channels" "" l} ge1}
  {wall float 0.02 {"wall\nthickness [cm]" "thickness of the wall between channels" "" m} ge0}
  {dia float 7.1 {"diameter [cm]" "diameter of the shadowing cylinder" "" r} gt0}
  {rot float 500 {"rotations\nper second" "frequency of rotation" "" n}}
  {phase float 0 {"phase [deg]" "dephasing angle at zero time" "" q}}
}

set chop2Add {
  {time_zero radio no {
    "set zero time"
    "yes: chopper defines zero time\nno: zero time is defined in source" "" z}
    {yes no} {1 0}}
}

set chop3Add {
  {number_of_gates radio 4 {
    "number of gates"
    "4: number of gates representing the channels ideal for thermal and best for cold neutrons\n6: more accurate but slower\n8: most accurate but slowest" "" p}
    {4 6 8} {4 6 8}}
}

### chopper fermi_str
###
set chopper_fermi_strESET [concat $chop1Add {
} $chop2Add $chop3Add ]


### chopper fermi_cur
###
set chopper_fermi_curESET [concat $chop1Add {
  {cfL float 5 {"optimal\nwavelength [A]" "optimal wavelength to be transmitted at highest intensity.\nIf radius of curvature is fixed:\nlambda[A] = 314.8/radius_of_curvature[m]/frequency[Hz]" "" L} gt0}
} $chop2Add {
  {chan_shape radio circular {
    "channel shape"
    "circular: channels have circular shape\nideal: channels close to parabolic shape" "" g}
    {"ideal" "circular"} {1 2}}
  {geomfile pareditablefile ch_fermi_geom.dat {"geometry\nfile" "output file of the curved channel geometry (for scatter plot of the last two columns, first column: channel index, O = envelope) " "" G}}
  } $chop3Add ]


### ref file description
###
set refESET {
  {"Sample Parameters" header}
  {mx float 0 {"main position\nX [cm]"
    "Generally defines the reference point (origin) of the sample in the frame provided by the former module."} 1}
  {my float 0 {"main position\nY [cm]"
    "Generally defines the reference point (origin) of the sample in the frame provided by the former module."} 1}
  {mz float 0 {"main position\nZ [cm]"
    "Generally defines the reference point (origin) of the sample in the frame provided by the former module."} 1}
  {thick float 0.00001 {"thickness\nsample [cm]"
    "Thickness of the rectangular sample, i.e. perpendicular to refl. surface.\n It determines the range of depth in which the reflection is supposed to take place."} ge0 "" 1}
  {wid float 1 {"width\nsample [cm]"
    "Width of the rectangular sample (along y-axis for reflection angle 0)."} ge0 "" 1}
  {hei float 1 {"length\nsample [cm]"
    "Length of the rectangular sample (along x-axis for reflection angle 0)."} ge0 "" 1}
  {"Output Frame" header}
  {gen radio "standard defined frame" {"frame\ngeneration"
    "If and only if user defined frame has been selected, then horiz. and vertical angle and output frame origin x,y, and z must be specified, too."}
    {"user defined frame" "standard defined frame"}}
  {}
  {horang float 0 {"horizontal\nangle [deg]"
    "In case of user define frame, a rotation about the Z axis and then a rotation about the (new) Y axis defines a new reference orientation for the output neutrons."} -180 180 1}
  {vertang float 0 {"vertical\nangle [deg]"
    "In case of user define frame, a rotation about the Z axis and then a rotation about the (new) Y axis defines a new reference orientation for the output neutrons."} -180 180 1}
  {}
  {x float 0 {"X' [cm]"
    "The x position of the output frame origin in the original frame."}}
  {y float 0 {"Y' [cm]"
    "The y position of the output frame origin in the original frame."}}
  {z float 0 {"Z' [cm]"
    "The z position of the output frame origin in the original frame."}}
}

proc refCheckErr {{app _}} {
  set err 0
  if {[entryVal gen $app] == "user defined frame"} {
    foreach l {horang vertang x y z} {
      if {[entryVal $l $app] == ""} {
	showText "!Please specify all values for user defined frame"
	set err 1
	break
      }
    }
  }
  return $err
}


### monochromator analyser
###   flat crystal

set ma_flatESET {
  {"Monochromator Analyser" header}
  {parfile pareditablefile crys.par {"parameter file" "" "" P} r crs 1}
  {reprate int 1 {"repetition\nrate"
    "If this integer > 1, the neutron is used multiple times for better statistics." "" A} 1 1000000 1}
  {shoriz float 0.8 {"mosaic spread\nhoriz. [deg]"
    "Horizontal fwhm component of the 2-dimensional Gaussian mosaic distribution [deg]" "" m}
    ge0 "" 1}
  {svert float 0.8 {"mosaic spread\nvert. [deg]"
    "Vertical fwhm component of the 2-dimensional Gaussian mosaic distribution [deg]" "" M}
    ge0 "" 1}
  {dspread float 0.00005 {"d spread"
    "Fwhm of the d-spacing distribution function divided by the lattice parameter under consideration. It is zero for a perfect crystal. " "" D} ge0 "" 1}
  {refl float 1 {"reflectivity\nnormalization [-]" "By this variable the peak reflectivity R may be renormalized from the\ndefault value (Pmax = 1)e.g. to (Pmax = 0.30), if R = 30%." "" R} gt0 "" 1}
  {}
  {dist radio Lorentzian {d-distribution "defines the d-spacing distribution function" "" d}
    {Lorentzian Gaussian} {1 2}}
}

### Monochromator analyser
###   focus initialization
set ma_focusESET [concat [globVal ma_flatESET] {
  {focus_file pareditablefile lamb_foc.dat {"focus file" "" "" G} w "" 1}
  {cevnum int 18 {"number of CE\nvertical" "The number of rows of the created crystal element-matrix." "" V} gt0 "" 1}
  {cradius float 200 {"radius\n[cm]"
    "Distance from the sample center to the bottom row of the crystal element-matrix." "" r} ge0 "" 1}
  {cangle float 0 {"angle\nvert. [deg]"
    "Angular offset of the bottom row of the crystal element-matrix relative to the horizontal plane containing the sample center." "" a} 1}
  {cehnum int 10 {"number of CE\nhorizontal" "The number of columns of the created crystal element-matrix." "" H} gt0 "" 1}
  {chradius float 200 {"radius\nhoriz. [cm]"
    "Radius of focussing in horizontal direction for a double focussing cylindrical shape." "" s} ge0 "" 1}
  {fopt radio "constant lambda" {"focusing option" "choose the focusing geometry" "" g}
    {"constant lambda" spherical "vert. cylinder" "double focussing"} {1 2 3 4}}
}]

### Monochromator analyser
###    external focus file
set ma_focus_datESET [concat [globVal ma_flatESET] {
  {focus_file pareditablefile lamb_foc.dat {"focus file" "External focus geometry file (must be provided to consider a crystal element-geometry which differs from the arrangement which can be automatically generated by using the module ma_focus)." "" G} r "" 1}
}]

### Monochromator analyser
###   crs file description

set crsESET {
  {"Monochromator-Analyser parameters" header}
  {mposx float 100 {"main position\nX [cm]" "Generally defines the reference point (origin) of the monochromator/analyser-system in the frame provided by the former module."} 1}
  {mposy float 0  {"main position\nY [cm]" "Generally defines the reference point (origin) of the monochromator/analyser-system in the frame provided by the former module."} 1}
  {mposz float 0  {"main position\nZ [cm]" "Generally defines the reference point (origin) of the monochromator/analyser-system in the frame provided by the former module."} 1}
  {offahoriz float 0 {"surface offset\nhorizontal [deg]" "A rotation first around the Z axis and then around the (new) Y axis gives a proper orientation of the crystal surface. 0 angle means perpendicular to beam."} 1}
  {offavert float 0 {"surface offset\nvertical [deg]" "A rotation first around the Z axis and then around the (new) Y axis gives a proper orientation of the crystal surface. 0 angle means perpendicular to beam."} 1}
  {}
  {bragghoriz float "" {"Bragg offset\nhorizontal [deg]" "horizontal offset from backscattering of the diffraction planes determining the Bragg angle. 0 angle means diffraction planes perpendicular to beam."}}
  {braggvert float "" {"Bragg offset\nvertical [deg]" "vertical offset from backscattering of the diffraction planes determining the Bragg angle. 0 angle means diffraction planes perpendicular to beam."}}
  {}
  {thick float 0.2 {"thickness cryst.\nelement [cm]"
    "Thickness, width and height give depth, horizontal and vertical dimensions of the rectangular crystal element."} gt0 "" 1}
  {width float 1 {"width cryst.\nelement [cm]"} gt0 "" 1}
  {height float 1 {"height cryst.\nelement [cm]"} gt0 "" 1}
  {dspacing float 3.135 {"d-spacing [A]"
    "Lattice parameter corresponding to a reflection from a (h,k,l) crystal plane."} gt0 "" 1}
  {reford int 1 {"order of\nreflection" "Order of reflection conforming to Bragg's Law."} ge1 "" 1}
  {}
  {mrange float 100 {"mosaic\nrange factor" "Sets the randomly covered angular range on the cone described by the mosaic normal vector, the axis being the wavevector of the neutron (cf. help manual)."} ge0 "" 1}
  {drange float 10 {"d-range factor"
    "Sets the range randomly covered by the lattice parameter."} gt0 "" 1}
  {"Output frame" header}
  {oframedef radio "standard frame generation"
    {"output frame definition" "If and only if \"user defined frame\" has been selected, then the following 5 entries must be specified, too"}
    {"standard frame generation" "user defined frame"} {0 1}}
  {}
  {oframex float 200 {"X' [cm]" "The x position of the output frame origin in the original frame."}}
  {oframey float 0 {"Y' [cm]" "The y position of the output frame origin in the original frame."}}
  {oframez float 0 {"Z' [cm]" "The z position of the output frame origin in the original frame."}}
  {oframehang float 180 {"horizontal\nangle [deg]" "In case of 'user defined output frame', a rotation about the Z axis and then a rotation about the (new)Y axis defines a new reference orientation for the output neutrons."}}
  {oframevang float 0 {"vertical\nangle [deg]" "In case of 'user defined output frame', a rotation about the Z axis and then a rotation about the (new)Y axis defines a new reference orientation for the output neutrons."}}
}


proc crsCheckErr {{app _}} {
  foreach l {oframedef oframehang oframevang oframex oframey oframez} {
    upvar #0 $l$app $l
  }
  if {$oframedef == "user defined frame" && \
	  ($oframehang == "" || $oframevang == "" || \
	       $oframex == "" || $oframey == "" || $oframez == "")} {
    showText "!Please specify last five entries for user defined frame"
    return 1
  }
  return 0
}

### polariser
###        he3
set polariser_he3ESET {
  {wdep radio analytical
    {"wavelength\ndependence" "choice between analytical or numerical definition of the wavelength dependence of the polarisation and transmission" "" a}
    {numerical analytical} {0 1}}
  {}
  {phe3 float 50 {"polarisation\nHe3 [%]" "active if a=1, polarisation of the He3 in %" "" b} gt0 lt100}
  {polx float 2944.8 {"polarisation\nxsection [barn]" "active if a=1, polarisation cross section of neutrons in He3 in barn" "" c} gt0}
  {dens float 0.1e19 {"density\nHe3 [1/cm3]" "active if a=1, density of the He3 gas in 1/cm3" "" d} gt0}
  {}
  {pfile pareditablefile "" {"polarisation\nfile" "data file for the wavelength dependent polarisation" "" P}}
  {tfile pareditablefile "" {"transmission\nfile" "data file for the wavelength dependent transmission" "" T}}
  {}
  {x float 100 {"position\nmain X [cm]" "x center position of the cylindrical chamber" "" k}}
  {y float 0   {"position\nmain Y [cm]" "y center position of the cylindrical chamber" "" l}}
  {z float 0   {"position\nmain Z [cm]" "z center position of the cylindrical chamber" "" m}}
  {}
  {clen float 100 {"cylinder\nlength [cm]" "length of the cylindrical chamber" "" X} gt0}
  {crad float 5   {"cylinder\nradius [cm]" " radius of the cylindrical chamber" "" Y} gt0}
  {}
  {gx float 1 {"guide field\nX [Oe]" "x component of the guide field in Oe" "" G}}
  {gy float 0 {"guide field\nY [Oe]" "y component of the guide field in Oe" "" H}}
  {gz float 0 {"guide field\nZ [Oe]" "z component of the guide field in Oe" "" K}}
  {px float 10 {"pol. field\nX [Oe]" "x component of the field in the chamber which is added to the guide field in Oe" "" M}}
  {py float 0 {"pol. field\nY [Oe]" "y component of the field in the chamber which is added to the guide field in Oe" "" N}}
  {pz float 0 {"pol. field\nZ [Oe]" "z component of the field in the chamber which is added to the guide field in Oe" "" O}}
  {ox float 200 {"output\nX [cm]" "x position of the output frame (in the input frame)" "" p}}
  {oy float 0   {"output\nY [cm]" "y position of the output frame (in the input frame)" "" r}}
  {oz float 0   {"output\nZ [cm]" "z position of the output frame (in the input frame)" "" s}}
}

### polarising
### mirror
gSet pol_mirrorESET {
  {pm_ufile pareditablefile mirr3+.dat {"Up-reflectivity\nfile" "reflectivity data file for Up neutrons" "" U}}
  {pm_dfile pareditablefile mirr1a.dat {"Down-reflectivit\nfile" "reflectivity data file for Down neutrons" "" D}}
  {pm_obs radio transmission {"mode" "choose between measuring in reflection and transmission" "" T}
    {reflection transmission} {0 1}}
  {"Mirror size" header}
  {pm_dx float 60 {"length [cm]" "length of the polarising mirror (along beam axis)" "" L} gt0 "" 1}
  {pm_dy float 10 {"width or\nheight [cm]" "width or height of the polarising mirror" "" W} gt0 "" 1}
  {"Mirror position and orientation" header}
  {pm_ori radio horizontal {"orientation" "choose between vertical and horizontal orientation of the mirror" "" O}
    {horizontal vertical} {0 1}}
  {pm_x float 100 {"position\nX [cm]" "x center position of the polarizing mirror" "" X}}
  {pm_y float 0   {"position\nY [cm]" "y center position of the polarizing mirror" "" Y}}
  {pm_z float 0   {"position\nZ [cm]" "z center position of the polarizing mirror" "" Z}}
  {pm_voff float 1 {"inclination [deg]" "rotation angle of the polarizing mirror" "" V}}
  {"Analysis direction" header}
  {pm_ax float 1 {"analysis dir.\nX [-]" "x direction vector component of the quantization direction" "" a}}
  {pm_ay float 0 {"analysis dir.\nY [-]" "y direction vector component of the quantization direction" "" b}}
  {pm_az float 0 {"analysis dir.\nZ [-]" "z direction vector component of the quantization direction" "" c}}
  {"Output frame" header}
  {pm_ox float 200 {"output\nX [cm]" "x position of the output frame (in the input frame)" "" x}}
  {pm_oy float 0   {"output\nY [cm]" "y position of the output frame (in the input frame)" "" y}}
  {pm_oz float 0   {"output\nZ [cm]" "z position of the output frame (in the input frame)" "" z}}
  {pm_r1 float 0 {"horiz. rotation\nangle [deg]" "rotation angle of the output frame in horizontal direction (first rotation)" "" h}}
  {pm_r2 float 0 {"vert. rotation\nangle [deg]" "rotation angle of the output frame in vertical direction (second rotation)" "" v}}
}

### polariser
###        sm
set polariser_smESET {
  {pfile pareditablefile polariser_SM.par
    {"parameter\nfile" "" "" P} w pol 1}
  {ufile pareditablefile mirr3+.dat {"Up-reflectivity\nfile" "reflectivity data file for Up neutrons" "" U}}
  {dfile pareditablefile mirr1a.dat {"Down-reflectivit\nfile" "reflectivity data file for Down neutrons" "" D}}
  {}
  {x float 100 {"position\nX [cm]" "x center position of the rectangular geometry polariser" "" a}}
  {y float 0   {"position\nY [cm]" "y center position of the rectangular geometry polariser" "" b}}
  {z float 0   {"position\nZ [cm]" "z center position of the rectangular geometry polariser" "" c}}
  {hoff float 0 {"horizontal\noffset [deg]" "rotation angle of the polariser in horizontal (first rotation) direction (0, 0 means parallel to X i.e. beam)" "" H}}
  {voff float 1 {"vertical\noffset [deg]" "rotation angle of the polariser in vertical (first rotation) direction (0, 0 means parallel to X i.e. beam" "" V}}
  {}
  {ox float 200 {"output\nX [cm]" "x position of the output frame (in the input frame)" "" R}}
  {oy float 0   {"output\nY [cm]" "y position of the output frame (in the input frame)" "" E}}
  {oz float 0   {"output\nZ [cm]" "z position of the output frame (in the input frame)" "" G}}
  {r1 float 0 {"horiz. rotation\nangle [deg]" "rotation angle of the output frame in horizontal direction (0, 0 means parallel to original X)" "" h}}
  {r2 float 0 {"vert. rotation\nangle [deg]" "rotation angle of the output frame in vertical direction (0, 0 means parallel to original X)" "" v}}
}



### pol file description

set polESET {
  {dx float 60 {"dimension\nX [cm]" "length of the polariser"} gt0}
  {dy float 10 {"dimension\nY [cm]" "width of the polariser"} gt0}
  {dz float 10 {"dimension\nZ [cm]" "height of the polariser"} gt0}
  {nc int 9    {"number of\nchannels" "number of channels in vertical direction"} ge1}
  {dw float 0.05 {"wall\nwidth [cm" "width of the wall between the channels"} gt0}
  {}
  {gx float 1 {"guide field\nX [Oe]" "x component of the guide field"}}
  {gy float 0 {"guide field\nY [Oe]" "y component of the guide field"}}
  {gz float 0 {"guide field\nZ [Oe]" "z component of the guide field"}}
  {ax float 1 {"analysis dir.\nX [-]" "x direction vector component of the quantization direction"}}
  {ay float 0 {"analysis dir.\nY [-]" "y direction vector component of the quantization direction"}}
  {az float 0 {"analysis dir.\nZ [-]" "z direction vector component of the quantization direction"}}
}


### flipper_coil
###
set flipper_coilESET {
  {"Rectangular coil flipper" header}
  {x float 5 {"position\nmain X [cm]" "center position x of the rectangular coil" "" k}}
  {y float 0 {"position\nmain Y [cm]" "center position y of the rectangular coil" "" l}}
  {z float 0 {"position\nmain Z [cm]" "center position z of the rectangular coil" "" m}}
  {cas radio "Y direction"
    {"coil axis\nshows" "orientation of the coil axes " "" y}
    {"Y direction" "Z direction"} {0 1}}
  {}
  {hof float 0 {"offset\nhoriz. [deg]" "rotation angle (first rotation) of the coil axes in horizontal direction" "" i}}
  {vof float 0 {"offset\nvert. [deg]"  "rotation angle (first rotation) of the coil axes in vertical direction" "" j}}
  {}
  {dx float 10 {"dimension\nX [cm]" "dimension of the rectangular coil in X direction" "" X}}
  {dy float 10 {"dimension\nY [cm]" "dimension of the rectangular coil in Y direction" "" Y}}
  {dz float 10 {"dimension\nZ [cm]" "dimension of the rectangular coil in Z direction" "" V}}
  {gf float 0.9787 {"guide\nfield [Oe]" "strength of the guide magnetic field which is considered parallel to the beam axes" "" G}}
  {}
  {cf float 0.9787  {"coil field\ncomponent [Oe]" "strength of the coil magnetic field which is considered parallel to the coil axes" "" H}}
  {wt float 0.2 {"wall\nthickness [cm]" "thickness of the coil wire (wall)" "" t} ge0}
  {ns float 10 {"field mesh\nsteps" "number of 'boxes' in which the field is devided (max:100)" "" N}}
  {ox float 10 {"output\nX [cm]" "x position of the output frame (in the input frame)" "" p}}
  {oy float 0 {"output\nY [cm]" "y position of the output frame (in the input frame)" "" r}}
  {oz float 0 {"output\nZ [cm]" "z position of the output frame (in the input frame)" "" s}}
}

### flipper_gradient
###
set BigFrameflipper_gradient 1

set flipper_gradientESET {
  {"Geometry description of the precession volume" header}
  {fx float 10 {"common\nfield X [cm]" "X Dimension of the flipper" "" X} gt0}
  {fy float 10 {"common\nfield Y [cm]" "Y Dimension of the flipper" "" Y} gt0}
  {fz float 10 {"common\nfield Z [cm]" "Z Dimension of the flipper" "" V} gt0}

  {px float 5 {"position\ncenter X [cm]" "Center position of the flipper" "" k}}
  {py float 0 {"position\ncenter Y [cm]" "Center position of the flipper" "" l}}
  {pz float 0 {"position\ncenter Z [cm]" "Center position of the flipper" "" m}}

  {"Rotation of Precession Volume" header}
  {rotproc float 0 {"horizontal\noffset [deg]" "Horizontal (around axis OZ) angle of the field volume" "" i}}

  {"Output Frame" header}
  {ox float 10 {"output\nframe X [cm]" "Position of the output frame (in the input frame)" "" p}}
  {oy float  0 {"output\nframe Y [cm]" "Position of the output frame (in the input frame)" "" r}}
  {oz float  0 {"output\nframe Z [cm]" "Position of the output frame (in the input frame)" "" s}}
  {"Number of domains" header}
  {nx int 4 {"in X\ndirection" "Number of domains in the X direction" "" C} gt0}
  {ny int 2 {"in Y\ndirection" "Number of domains in the Y direction" "" D} gt0}
  {nz int 2 {"in Z\ndirection" "Number of domains in the Z direction" "" E} gt0}
  {"Rotating Magnetic Field" header}
  {}
  {rax radio 0X {"rotating field\naxis" "Rotating field around given axis OX, OY or OZ, values 0,1,2" "" M}
    {0X 0Y 0Z} {0 1 2}}
  {mf float 5000 {"magnetic field\namplitude [Oe]" "Strength or amplitude of the rotating magnetic field, Oe=Gauss" "" d} ge0}
  {rf float 300000 {"rotation\nfrequency [Hz]" "Rotation frequency of the magnetic field" "" w}}
  {chgampl radio sinus {"amplitude\nchanging by" "Amplitude of rotating magnetic field is changing by sinus (with semi-period - appropriate dimensions of rotating field volume) law, permanently and by solinoid formula (not yet active)" "" h}
    {sinus permanent solenoid} {0 1 2}}
  {bp float 0 {"begin phase\n[deg] " "Initial phase for the rotating field" "" z}}
  {raxx radio 0X {"amplitude changing\nalong axis" "Key-Direction for changing of amplitude of rotating field along given axis OX, OY or OZ, values 0,1,2. Actually, if amplitude changing by sinus was chosen" "" y}
    {0X 0Y 0Z} {0 1 2}}
  {}
  {mfd float 0 {"deviation of\namplitude [%]" "Deviation of amplitude of the rotating magnetic field in percent" "" a} ge0}
  {distra radio Uniform {"amplitude distribution" "Distribution of random values: amplitude of the rotating magnetic field" "" e} {Normal Uniform} {0 1}}
  {rfd float 0 {"deviation of\nfrequency [%]" "Deviation of Rotation frequency of the magnetic field in percent" "" b} ge0}
  {}
  {distrf radio Uniform {"Frequency distribution" "Distribution of random values: frequency of the rotating magnetic field" "" v}
    {Normal Uniform} {0 1}}
  {tofprec radio yes {"TOF from\nprec. module" "Use or do not use the neutrons TOF from preceding modules for rotating magnetic field phase" "" n}
    {yes no} {1 0}}

  {"Guide Magnetic Field" header}
  {chgamplgui radio cosinus {"law of changing" "Laws of distribution of guide magnetic field: cosine law (with semi-period - appropriate dimensions of rotating field volume), linearly and pernanently" "" u} {cosinus linear permanent} {0 1 2}}
  {chguidedch radio 0X {"amplitude\nchanging\nalong axis" "Key-Direction for amplitude changing of guide field along given axis OX, OY or OZ, values 0,1,2. Actually if cosinus law of changing was chosen" "" t} {0X 0Y 0Z} {0 1 2}}

  {pmx float 0 {"perm. / initial\ncomponent X [Oe]" "Permanent (for cosine amd permanent laws) or initial (for linear law) value of the X component (projection in the axis 0X) of the guide magnetic field, Oe=Gauss" "" I}}
  {pmy float 0 {"perm. / initial\ncomponent Y [Oe]" "Permanent (for cosine and permanent laws) or initial (for linear law) value of the Y component (projection in the axis 0Y) of the guide magnetic field, Oe=Gauss" "" A}}
  {pmz float 0 {"perm. / initial\ncomponent Z [Oe]" "Permanent (for cosine and permanent laws) or initial (for linear law) value of the Z component (projection in the axis 0Z) of the guide magnetic field, Oe=Gauss" "" K}}

  {plmx float 0 {"amplitude or\nfinal X [Oe]" "Amplitude (for cosine law) or final value (for linear law) of the X component (projection in the axis 0X) of the guide magnetic field, Oe=Gauss" "" P}}
  {plmy float 0 {"amplitude or\nfinal Y [Oe]" "Amplitude (for cosine law) or final value (for linear law) of the Y component (projection in the axis 0Y) of the guide magnetic field, Oe=Gauss" "" Q}}
  {plmz float 0 {"amplitude or\nfinal Z [Oe]" "Amplitude (for cosine law) or final value (for linear law) of the Z component (projection in the axis 0Z) of the guide magnetic field, Oe=Gauss" "" R}}

  {pmde float 0 {"additional random\n magnetic field, [Oe]" "Amplitude of the additional random magnetic field" "" q} ge0}

  {"Addition options" header}
  {outkey radio no {"output results" "Output intermediately results of simulations in the file RELATIVE OX axis" "" S} {yes no} {1 0}}
  {bfp pareditablefile revp.dat {"output file:\npolarisation" "Name of file for: results - polarisation" "" O}}
  {bff pareditablefile revm.dat {"output file:\nmagneticfield" "Name of file for output results - magnetic field" "" N}}
}


### resonator_drabkin
###
set resonator_drabkinESET {
  {"Geometry description of the flipper" header}
  {fx float 10 {"resonator X [cm]" "Dimension of the common precession volume - resonator size" "" X} gt0}
  {fy float 10 {"resonator Y [cm]" "Dimension of the common precession volume - resonator size" "" Y} gt0}
  {fz float 10 {"resonator Z [cm]" "Dimension of the common precession volume - resonator size" "" V} gt0}

  {px float 5 {"position\ncenter X [cm]" "Center position of the resonator" "" k}}
  {py float 0 {"position\ncenter Y [cm]" "Center position of the resonator" "" l}}
  {pz float 0 {"position\ncenter Z [cm]" "Center position of the resonator" "" m}}

  {"Output Frame" header}
  {ox float 10 {"output\nframe X [cm]" "Position of the output frame (in the input frame)" "" p}}
  {oy float  0 {"output\nframe Y [cm]" "Position of the output frame (in the input frame)" "" r}}
  {oz float  0 {"output\nframe Z [cm]" "Position of the output frame (in the input frame)" "" s}}
  {"Number of domains" header}
  {nx int 20 {"in X direction" "Number of domains in the X direction" "" C} gt0}
  {ny int 2 {"in Y direction" "Number of domains in the Y direction" "" D} gt0}
  {nz int 2 {"in Z direction" "Number of domains in the Z direction" "" E} gt0}
  {"Periodical Magnetic Field" header}
  {}
  {rax radio 0Y {"periodical field\nparallel of axis" "Periodical field is parallel to the axis OX, OY or OZ, values 0,1,2" "" M} {0X 0Y 0Z} {0 1 2}}
  {mf float 500 {"magnetic\nfield - amplitude [Oe]" "Strength or amplitude of the periodical magnetic field, Oe=Gauss" "" d} ge0}
  {raxdi radio Uniform {"periodical field\nchaning law" "The law of changing of the periodical magnetic field" "" v} {Uniform Sinus Gauss} {0 1 2}}
  {}
  {mfd float 0 {"Deviation of\namplitude [%]" "Deviation of amplitude of the periodical magnetic field in percent" "" a} ge0}
  {distra radio Uniform {"Amplitude distribution" "Distribution of random values: amplitude of the periodical magnetic field" "" e} {Normal Uniform} {0 1}}
  {mfds float 1 {"Sigma for\n gauss distr. [Oe]" "Sigma for gauss distribution of amplitude of the periodical magnetic field" "" x} gt0}

  {"Guide Magnetic Field" header}
  {pmx float 0 {"component\nX [Oe]" "X component (projection in the axis 0X) of the permanent magnetic field, Oe=Gauss" "" I}}
  {pmy float 0 {"component\nY [Oe]" "Y component (projection in the axis 0Y) of the permanent magnetic field, Oe=Gauss" "" A}}
  {pmz float 0 {"component\nZ [Oe]" "Z component (projection in the axis 0Z) of the permanent magnetic field, Oe=Gauss" "" K}}

  {pmde float 0 {"additional\nrandom\nmagnetic field [Oe]" "Amplitude of the additional random magnetic field" "" q} ge0}

  {"Addition options" header}
  {outkey radio no {"output results" "Output intermediately results of simulations in the file RELATIVE OX axis" "" S} {yes no} {1 0}}
  {bfp pareditablefile revp.dat {"output file:\npolarisation" "Name of file for: results - polarisation" "" O}}
  {bff pareditablefile revm.dat {"output file:\nmagneticfield" "Name of file for output results - magnetic field" "" N}}
}


### precessionfield
###
set precessionfieldESET {
  {"Common options" header}
  {bf pareditablefile magneticmap.dat {"field map file" "data file giving the field map which is read in externally" "" P}}
  {"Field options" header}
  {inh radio inhomogeneous
    {"field option" "magnetic field option" "" O}
    {inhomogeneous homogeneous} {1 0}}
  {}
  {x float 5 {"position\nmain X [cm]" "center x position of the field map " "" k}}
  {y float 0 {"position\nmain Y [cm]" "center y position of the field map " "" l}}
  {z float 0 {"position\nmain Z [cm]" "center z position of the field map " "" m}}
  {}
  {ho float 0 {"offset\nhoriz. [deg]" "horizontal (first rotation) rotation angle of the field map" "" i}}
  {vo float 0 {"offset\nvert. [deg]"  "vertical (first rotation) rotation angles of the	field map" "" j}}
  {}
  {ox float 10 {"output\nX [cm]" "x position of the output frame (in the input frame)" "" p}}
  {oy float 0  {"output\nY [cm]" "y position of the output frame (in the input frame)" "" r}}
  {oz float 0  {"output\nZ [cm]" "z position of the output frame (in the input frame)" "" s}}
  {"Homogeneous field case" header}
  {dx float 10 {"dimension\nfield X [cm]" "active if homogeneous field, gives x dimension of the precession volume" "" X}}
  {dy float 10 {"dimension\nfield Y [cm]" "active if homogeneous field, gives y dimension of the precession volume" "" Y}}
  {dz float 10 {"dimension\nfield Z [cm]" "active if homogeneous field, gives z dimension of the precession volume" "" V}}
  {mx float 1 {"magnetic\nfield X [Oe]" "x component of the magnetic field in Oe" "" T}}
  {my float 0 {"magnetic\nfield Y [Oe]" "y component of the !magnetic field in Oe" "" G}}
  {mz float 0 {"magnetic\nfield Z [Oe]" "z component of the magnetic field in Oe" "" H}}
}

### rotating field
### Options: X Y V  k l m  i p r s  C D E  M d w z a e t b v n  I A K q W S x O N
###
set BigFramerotating_field 1

set rotating_fieldESET {
  {"Geometry Description of the Precession Volume" header}
  {fx float 10 {"dim. of common\nfield X [cm]" "Dimension of the common precession volume" "" X} gt0}
  {fy float 10 {"dim. of common\nfield Y [cm]" "Dimension of the common precession volume" "" Y} gt0}
  {fz float 10 {"dim. of common\nfield Z [cm]" "Dimension of the common precession volume" "" V} gt0}
  {px float 5 {"position\nmain X [cm]" "Center position of the field map" "" k}}
  {py float 0 {"position\nmain Y [cm]" "Center position of the field map" "" l}}
  {pz float 0 {"position\nmain Z [cm]" "Center position of the field map" "" m}}

  {"Rotation of Precession Volume" header}
  {rotproc float 0 {"horizontal\noffset [deg]" "Horizontal (around axis OZ) angle of the field volume" "" i}}
  {"Output Plane" header}
  {ox float 10 {"output\nX [cm]" "position of the output frame (in the input frame)" "" p}}
  {oy float  0 {"output\nY [cm]" "position of the output frame (in the input frame)" "" r}}
  {oz float  0 {"output\nZ [cm]" "position of the output frame (in the input frame)" "" s}}
  {"Number of Domains"  header}
  {nx int 40 {"domains in\nX direction" "Number of domains in the X direction" "" C} gt0}
  {ny int 20 {"domains in\nY direction" "Number of domains in the Y direction" "" D} gt0}
  {nz int 20 {"domains in\nZ direction" "Number of domains in the Z direction" "" E} gt0}
  {"Rotating Magnetic Field" header}
  {rax radio 0X {"rotating  field\naxis" "Rotating field around given axis OX, OY or OZ, values 0,1,2" "" M}
  {0X 0Y 0Z} {0 1 2}}
  {mf float 5000 {"magnetic field\namplitude [Oe]" "Strength or amplitude of the rotating magnetic field, Oe=Gauss" "" d} ge0}
  {rf float 300000 {"rotation\nfrequency [Hz]" "Rotation frequency of the magnetic field" "" w}}
  {bp float 0 {"begin phase\n[deg] " "Initial phase for the rotating magnetic field" "" z}}
  {devamp float 0 {"deviation of\namplitude [%]" "Deviation of amplitude of the rotating magnetic field in percent" "" a} ge0}

  {disamp radio uniform {"amplitude\ndistribution" "Distribution of amplitude of the rotating magnetic field along flight direction" "" e}
    {normal_ran uniform_ran normal uniform from_file} {0 1 2 3 4}}

  {inampld pareditablefile ampld.dat {"file amplitude\ndistribution" "Name of file for describing of the amplitude distribution " "" t}}

  {deffreq float 0 {"deviation of\nfrequency [%]" "Deviation of Rotation frequency of the magnetic field in percent" "" b} ge0}

  {disfreq radio uniform {"frequency\ndistribution" "Distribution of frequency of the rotating  magnetic field" "" v}
    {normal_ran uniform_ran uniform} {0 1 2}}

  {tofprec radio yes {"TOF from\nprec. module" "Use or do not use the neutrons TOF from preceding modules for the rotating field phase" "" n}
    {yes no} {1 0}}
  {"Permanent Magnetic Field" header}
  {pmx float 0 {"component\nX [Oe]" "X component (projection in the axis) of the permanent magnetic field, Oe=Gauss" "" I}}
  {pmy float 0 {"component\nY [Oe]" "Y component (projection in the axis) of the permanent magnetic field, Oe=Gauss" "" A}}
  {pmz float 0 {"component\nZ [Oe]" "Z component (projection in the axis) of the permanent magnetic field, Oe=Gauss" "" K}}
  {addrand float 0 {"additional random\nmagnetic field, [Oe]" "Amplitude of the additional random magnetic field" "" q} ge0}
  {"Additional Options" header}
  {calcwav float 20.0 {"wavelength\nfor calc. [A]" "Wavelength for caluculation of conditions for PI-flipping" "" W} gt0}
  {ores radio no {"output results" "Output intermediately results of simulations in the file" "" S}
    {yes no} {1 0}}
  {fieldcalc radio no {"rotating field\ncalculation" "Calculate of amplitude of rotating field according given wavelength (Calc. wavelength) and dimension of common field X (depth), values 0,1" "" x}
    {no yes} {0 1}}
  {opolout pareditablefile revp.dat {"output file\npolarisation" "Name of file for: results - polarisation" "" O}}
  {omag pareditablefile revm.dat {"output file\nmagnetic field" "Name of file for output results - magnetic field" "" N}}
  {btrap radio no {bootstrap "Use or do not use a bootstrap configuration" "" T} {yes no} {1 0}}
}

### visual
###
set visualESET {
  {visdev radio display {device "visual device" "" o} {display file display+file} {1 2 3}}
  {vt radio "circular beam" {type "type of\nvisualization" "" R}
    {"circular beam" "rectangular beam" "time_of_arrival(wavelength)" "wavelength(time_of_arrival)"}
    {1 2 3 4}}
  {}
  {wr float 10 {"window\nradius [cm]" " Radius of circle, which will appear" "" r} gt0}
  {y float 0 {"circle center\nY [cm]" "Y coordinate of circle center" "" y}}
  {z float 0 {"circle center\nZ [cm]" "Z coordinate of circle center" "" z}}
  {miv float 0.1 {"minimal\nwavelength [A]" "" "" m}}
  {mav float 10 {"maximal\nwavelength [A]" "" "" M}}
  {}
  {mit float 0 {"minimal\ntime value [ms]" "" "" t}}
  {mat float 20 {"maximal\ntime value [ms]" "" "" T}}
  {"Rectangular window" header}
  {hmi float -10 {"height\nmininum [cm]" "" "" h}}
  {hma float 10  {"height\nmaximum [cm]" "" "" H}}
  {}
  {wmi float 0.1 {"width\nminimum [cm]" "" "" w}}
  {wma float 10  {"width\nmaximum [cm]" "" "" W}}
  {sel radio all
    {"wavelength\nselection" "wavelength selection: all or only those neutrons within wavelength within range [wavemin,wavmax]" "" k}
    {all "only within range"} {0 1}}
}


### Monitor many many modules

proc genFE {n} {
  set ll {"monitor file"
    "the monitor output file: it contains the number of probability counts for each segment of the monitored interval" "" O}
  return [list [list monitor_file moneditablefile $n.dat $ll "" "" 1]]
}

proc genFE2 {n} {
  set ll {"monitor file"
    "the monitor output file: it contains the number of probability counts for each segment of the monitored interval" "" O}
  return [list [list monitor_file mon2editablefile $n.dat $ll "" "" 1]]
}

set dA {
  {"Analysis direction" header}
  {dirx float 1 {"direction\nX" "x component of the direction vector representing the quantization direction" "" a}}
  {diry float 0 {"direction\nY" "y component of the direction vector representing the quantization direction" "" b}}
  {dirz float 0 {"direction\nZ" "z component of the direction vector representing the quantization direction" "" c}}
}

set nA {
  {number_bins int 10 {
    "number\nof bins"
    "number of bins determines the segmentation of the interval" "" n} 1 99999 1}
  {mtrl_colour int 0 {
    "colour" "colour necessary for the trajectory to be evaluated\ncolour 0 means: all trajectories are evaluated" "" C} 0 32768}
}
set nnA {
  {withbin radio yes {"normalize\nwith binsize" "If activated, in each channel count-rate and standard deviation are normalised with the binsize on the wavelength, time-of-flight, etc axis." "" f} {yes no} {1 0}}
}

set mA {
  {}
  {min_w float 0 {
    "minimal\nwavelength [A]" "lower bound of the monitored interval" "" m} ge0 "" 1}
  {max_w float 20 {
    "maximal\nwavelength [A]" "upper bound of the monitored interval" "" M} gt0  "" 1}
  {reff pareditablefile "" {
    "reference file" "reference file: it contains input data that serve to normalize the monitor data" "" R}}
}

set eA {
  {}
  {min_w float 0 {
    "minimal\nenergy [meV]" "lower bound of the monitored interval" "" m} ge0 "" 1}
  {max_w float 20 {
    "maximal\nenergy [meV]" "upper bound of the monitored interval" "" M} gt0  "" 1}
}

set pA {
  {prob_w radio yes {
    "probability\nweight" "the neutron probability weights, e.g. mirroring the flux distribution of the source or the sample scattering processes, can be fixed to 1 for every neutron with \"no\"" "" p}
    {yes no} {1 0}}
  {excl_counts radio no {
    "exclusive\ncounts" "if \"exclusive counts\" is activated, then only the monitored neutrons will be considered by subsequent modules and/or written to the VITESS output file." "" e}
    {yes no} {1 0}}
}

set tA {
  {timevalbegin float "" {
    "time interval\nbegin [ms]" "begin of time interval to be evaluated" "" t}}
  {timevalend float "" {
    "time interval\nend [ms]" "end of time interval to be evaluated" "" T}}
}


### monitor
###   wavelength

set mon1_lambdaESET [concat [genFE lambda] $nA $nnA $mA $pA $tA]
proc mon1_lambdaCheckErr {{app _}} {
  return [checkMiMaErr min_w max_w "" $app]
}

set tA {
  {timevalbegin float -1.e10 {
    "time interval\nbegin [ms]" "begin of time interval to be evaluated" "" t}}
  {timevalend float 1.e10 {
    "time interval\nend [ms]" "end of time interval to be evaluated" "" T}}
}

set monpol_lambdaESET [concat [genFE p_lambda] $nA $mA $pA $tA $dA]
proc monpol_lambdaCheckErr {{app _}} {
  return [checkMiMaErr min_w max_w "" $app]
}


### monitor
###   energy

set mon1_energyESET [concat [genFE energy] $nA $nnA $eA $pA $tA]
proc mon1_energyCheckErr {{app _}} {
  return [checkMiMaErr min_e max_e "" $app]
}


### monitor
###   time
set mA {
  {}
  {min_time float 0 {
    "minimal\ntime [ms]" "lower bound of the monitored interval" "" m} 1}
  {max_time float 20 {
    "maximal\ntime [ms]" "upper bound of the monitored interval" "" M} 1}
}

set mon1_timeESET [concat [genFE time] $nA $nnA $mA $pA]
proc mon1_timeCheckErr {{app _}} {
  return [checkMiMaErr min_time max_time "" $app]
}

set monpol_timeESET [concat [genFE p_time] $nA $mA $pA $dA]
proc monpol_timeCheckErr {{app _}} {
  return [checkMiMaErr min_time max_time "" $app]
}

### monitor
###   divergence y

set mA {
  {}
  {min_div float -10 {
    "min. div.\nx <-> y [deg]" "lower bound of the monitored interval" "" m} 1}
  {max_div float 10 {
    "max. div.\nx <-> y [deg]" "upper bound of the monitored interval" "" M} 1}
}

set mon1_divyESET [concat [genFE divy] $nA $nnA $mA $pA]
proc mon1_divyCheckErr {{app _}} {
  return [checkMiMaErr min_div max_div "" $app]
}

set monpol_divyESET [concat [genFE p_divy] $nA $mA $pA $dA]
proc monpol_divyCheckErr {{app _}} {
  return [checkMiMaErr min_div max_div "" $app]
}

### monitor
###   divergence z
set mA {
  {}
  {min_div float -10 {
    "min. div.\nx <-> z [deg]" "lower bound of the monitored interval" "" m} 1}
  {max_div float 10 {
    "max. div.\nx <-> z [deg]" "upper bound of the monitored interval" "" M} 1}
}

set mon1_divzESET [concat [genFE divz] $nA $nnA $mA $pA]
proc mon1_divzCheckErr {{app _}} {
  return [checkMiMaErr min_div max_div "" $app]
}

set monpol_divzESET [concat [genFE p_divz] $nA $mA $pA $dA]
proc monpol_divzCheckErr {{app _}} {
  return [checkMiMaErr min_div max_div "" $app]
}


### monitor
###   mon1_y

set mA {
  {}
  {minv float -10 {
    "min. y [cm]" "lower bound of the monitored interval" "" m} 1}
  {maxv float 10 {
    "max. y [cm]" "upper bound of the monitored interval" "" M} 1}
}

set mon1_yESET [concat [genFE pos_y] $nA $nnA $mA $pA]
proc mon1_yCheckErr {{app _}} {
  return [checkMiMaErr minv maxv "" $app]
}

set monpol_yESET [concat [genFE p_pos_y] $nA $mA $pA $dA]
proc monpol_yCheckErr {{app _}} {
  return [checkMiMaErr minv maxv "" $app]
}

### monitor
###   mon1_z

set mA {
  {}
  {minv float -10 {
    "min. z [cm]" "lower bound of the monitored interval" "" m} 1}
  {maxv float 10 {
    "max. z [cm]" "upper bound of the monitored interval" "" M} 1}
}

set mon1_zESET [concat [genFE pos_z] $nA $nnA $mA $pA]
proc mon1_zCheckErr {{app _}} {
  return [checkMiMaErr minv maxv "" $app]
}

set monpol_zESET [concat [genFE p_pos_z] $nA $mA $pA $dA]
proc monpol_zCheckErr {{app _}} {
  return [checkMiMaErr minv maxv "" $app]
}

### mon2
###   position

set nA {
  {number_ybins int 10 {
    "number\nof y-bins" "number of bins within the y-axis interval" "" y} 1 200}
  {number_zbins int 10 {
    "number\nof z-bins" "number of bins within the z-axis interval" "" z} 1 200 1}
}
set mA {
  {}
  {min_y float -1000 {"minimal\ny-value [cm]" "" "" w} -10000 10000 1}
  {max_y float 1000 {"maximal\ny-value [cm]" "" "" W} -10000 10000 1}
  {}
  {min_z float -1000 {"minimal\nz-value [cm]" "" "" h} -10000 10000 1}
  {max_z float 1000 {"maximal\nz-value [cm]" "" "" H} -10000 10000 1}
}

set mon2_posESET [concat [genFE2 pos] $nA $mA $pA]
proc mon2_posCheckErr {{app _}} {
  if [checkMiMaErr min_y max_y "" $app] {return 1}
  return [checkMiMaErr min_z max_z "" $app]
}

set monitorpol_posESET [concat [genFE p_pos] $nA $mA $pA $dA]
proc monitorpol_posCheckErr {{app _}} {
  if [checkMiMaErr min_y max_y "" $app] {return 1}
  return [checkMiMaErr min_z max_z "" $app]
}

### mon2
###   div

set nA {
  {number_ybins int 10 {"number\nof y-bins" "" "" y} 1 200}
  {number_zbins int 10 {"number\nof z-bins" "" "" z} 1 200 1}
}
set mA {
  {}
  {min_y float -90 {"minimal\ny-value [deg]" "" "" w} -180 180 1}
  {max_y float 90 {"maximal\ny-value [deg]" "" "" W} -180 180 1}
  {}
  {min_z float -90 {"minimal\nz-value [deg]" "" "" h} -180 180 1}
  {max_z float 90 {"maximal\nz-value [deg]" "" "" H} -180 180 1}
}

set mon2_divESET [concat [genFE2 div] $nA $mA $pA]
proc mon2_divCheckErr {{app _}} {
  if [checkMiMaErr min_y max_y "" $app] {return 1}
  return [checkMiMaErr min_z max_z "" $app]
}

### mon2
###   kdiv

set mA {
  {}
  {min_ky float -0.1 {"minimal\nky-value [1/Ang]" "" "" w} -100 100 1}
  {max_ky float 0.1 {"maximal\nky-value [1/Ang]" "" "" W} -100 100 1}
  {}
  {min_kz float -0.1 {"minimal\nkz-value [1/Ang]" "" "" h} -100 100 1}
  {max_kz float 0.1 {"maximal\nkz-value [1/Ang]" "" "" H} -100 100 1}
}

set mon2_kdivESET [concat [genFE2 kdiv] $nA $mA $pA]
proc mon2_kdivCheckErr {{app _}} {
  if [checkMiMaErr min_ky max_ky "" $app] {return 1}
  return [checkMiMaErr min_kz max_kz "" $app]
}

### mon2
###   y_divy

set nA {
  {number_ybins int 10 {"number\nof y-bins" "" "" y} 1 200}
  {number_zbins int 10 {"number\nof divy-bins" "" "" z} 1 200 1}
}
set mA {
  {}
  {min_y float -1000 {"minimal\ny-value [cm]" "" "" w} -1000 1000 1}
  {max_y float 1000 {"maximal\ny-value [cm]" "" "" W} -1000 1000 1}
  {}
  {min_z float -10 {"minimal\ndivy-value [deg]" "" "" h} -90 90 1}
  {max_z float 10 {"maximal\ndivy-value [deg]" "" "" H} -90 90 1}
}

set mon2_y_divyESET [concat [genFE2 y_divy] $nA $mA $pA]
proc mon2_y_divyCheckErr {{app _}} {
  if [checkMiMaErr min_y max_y "" $app] {return 1}
  return [checkMiMaErr min_z max_z "" $app]
}

### mon2
###   z_divz

set nA {
  {number_ybins int 10 {"number\nof z-bins" "" "" y} 1 200}
  {number_zbins int 10 {"number\nof divz-bins" "" "" z} 1 200 1}
}
set mA {
  {}
  {min_y float -1000 {"minimal\nz-value [cm]" "" "" w} -1000 1000 1}
  {max_y float 1000 {"maximal\nz-value [cm]" "" "" W} -1000 1000 1}
  {}
  {min_z float -10 {"minimal\ndivz-value [deg]" "" "" h} -90 90 1}
  {max_z float 10 {"maximal\ndivz-value [deg]" "" "" H} -90 90 1}
}
set mon2_z_divzESET [concat [genFE2 z_divz] $nA $mA $pA]
proc mon2_z_divzCheckErr {{app _}} {
  if [checkMiMaErr min_y max_y "" $app] {return 1}
  return [checkMiMaErr min_z max_z "" $app]
}

### mon2
###   tof

set nA {
  {number_ybins int 10 {"number of\nTOF-bins" "" "" y} 1 200}
  {number_zbins int 10 {"number of\nwavelength-bins" "" "" z} 1 200 1}
}
set mA {
  {}
  {min_tof float -1000 {"minimal\ntof-value [ms]" "" "" w} -10000 10000 1}
  {max_tof float 1000 {"maximal\ntof-value [ms]" "" "" W} -10000 10000 1}
  {}
  {min_wl float 0.1 {
    "minimal\nwavelength [A]" "lower bound of the monitored interval" "" m} ge0 "" 1}
  {max_wl float 10 {
    "maximal\nwavelength [A]" "upper bound of the monitored interval" "" M} gt0  "" 1}
}

set mon2_tofwlESET [concat [genFE2 tof_wl] $nA $mA $pA]
proc mon2_tofwlCheckErr {{app _}} {
  if [checkMiMaErr min_tof max_tof "" $app] {return 1}
  return [checkMiMaErr min_wl max_wl "" $app]
}

### mon2
###   wldiv

set nA {
  {number_ybins int 10 {"number of\nwavelength-bins" "" "" y} 1 200}
  {number_zbins int 10 {"number of\ndivergence-bins" "" "" z} 1 200 1}
}
set mA {
  {}
  {min_wl float 0.1 {"minimal\nwavelength [A]" "" "" w} -10000 10000 1}
  {max_wl float 20 {"maximal\nwavelength [A]" "" "" W} -10000 10000 1}
  {}
  {min_div float -10 {"minimal\ndivergence [deg]" "lower bound of the monitored interval" "" h}}
  {max_div float 10 {"maximal\ndivergence [deg]" "upper bound of the monitored interval" "" H}}
  {}
  {conmin float -90 {"constrain\nmin [deg]"
    "this defines a constraint in the divergence perpendicular to the selected one" "" c}}
  {conmax float 90 {"constrain\nmax [deg]"
    "this defines a constraint in the divergence perpendicular to the selected one" "" C}}
  {conopt radio horizontal {"analysis\ndirection" "selects analysis direction" "" q}
    {horizontal vertical} {1 2}}
}

set mon2_wldivESET [concat [genFE2 wl_div] $nA $mA $pA]
proc mon2_tofwlCheckErr {{app _}} {
  if [checkMiMaErr min_wl max_wl "" $app] {return 1}
  return [checkMiMaErr min_div max_div "" $app]
}

### sample
###

set Refa "This option describes the solid angle covered by the detector. The direction (Theta,Phi) points to the middle of the covered area, which extend from \[Theta-dTheta; Theta+dTheta\] and \[Phi-dPhi;Phi+dPhi\].\nTheta is defined as the angle between +x-axis (main flight direction of the neutrons) and the Vector R to be described. Phi is the angle between the +y-axis and the projection of R into the yz-plane. x,y and z form right-handed system.\nIf you specify any parameter of Theta,dtheta,Phi, and dPhi, you must specify all of them. The default is a coverage of 4*PI."
set Refb "'repetitions' specifies the number of data sets (trajectories) generated for each scattered trajectory. A larger number of repetitions enriches the population on the detector and gives therefore better statistics in the spectrum."

set sampleASET [list \
  [list bdtheta float "" [list "Theta \[deg]"  $Refa "" D] 0 180] \
  [list dtheta float ""  [list "dTheta \[deg]" $Refa "" d] 0 90] \
  [list bphi float ""    [list "Phi \[deg]"    $Refa "" P] 0 360] \
  [list bdphi float ""   [list "dPhi \[deg]"   $Refa "" p] 0 180] \
  [list reprate int 1 [list repetitions $Refb "" A] 0 1000000 1] \
  {incoscat radio no {
    "incoherent\nscattering" "'yes' activates calculation of incoherent scattering" "" I}
    {yes no} {1 0}} \
]


### sample
###   file description for different sample types

set Refa "Position of the sample centre relative to the coordinate system defined by the preceding module."
set Refb "Component of the vector describing the orientation of the sample.
spherical geometry  : no values need to be provided
cylindrical geometry: default (0,0,1); vector describes orientation of the cylinder axis
cubic symmetry      : default (1,0,0) describes parallelepiped with thickness in x direction"

set samASET [list {Sample header} \
  [list x float "" [list "x \[cm]" $Refa] 1] \
  [list y float "" [list "y \[cm]" $Refa] 1] \
  [list z float "" [list "z \[cm]" $Refa] 1] \
  {} \
  {cyl radio cylinder {"sample\ngeometry"} {cylinder sphere cuboid} {cyl bal cub}} \
  {} \
  {thick float "" {"thickness or\nradius [cm]"
    "thickness of cuboid, or radius of sphere, or radius of cylinder"} gt0 "" 1} \
  {hei float "" {"height [cm]" "height of cuboid, or height of cylinder"} gt0} \
  {wid float "" {"width [cm]" "width of cuboid"} gt0} \
  {} \
  [list cx float "" [list "x direction" $Refb]] \
  [list cy float "" [list "y direction" $Refb]] \
  [list cz float "" [list "z direction" $Refb]] \
  ]

### sample
###   pow file description

set powESET [concat $samASET {
  {sfactfile pareditablefile "" {"structure\nfactor file"} r}
  {Scattering header}
  {tscat float "" {"incoherent scat-\ntering [1/cm]" "macroscopic cross-section"} 1}
  {cscat float "" {"total scat-\ntering [1/cm]"      "macroscopic cross-section"} 1}
  {absorp float "" {"absorption\n[1/cm]" "macroscopic cross-section (with respect to a wavelength of 1.798 A)"} 1}
  {vol float "" {"unit cell\nvolume [A^3]" "Unit cell volume in cubic Angstroem."} gt0 "" 1}
}]

### sample
###   psq file description

set psqESET [concat $samASET {
  {sfac radio "from file" {"structure\nfactor"} {"from file" "as function"} {D F}}
  {}
  {sfactfile editablefile "" {"structure\nfactor file"} r}
  {Scattering header}
  {tscat float "" {"incoherent scat-\ntering [1/cm]" "macroscopic cross-section"} 1}
  {cscat float "" {"coherent scat-\ntering [1/cm]" "macroscopic cross-section"} 1}
  {absorp float "" {"absorption\n[1/cm]"
    "macroscopic cross-section (with respect to a wavelength of 1.798 A)"} 1}
}]

proc checkMv {v name rc} {
  if {$v == ""} {
    upvar $rc status
    showText "!Please specify a value for $name"
    set status 1
  }
}

proc samplefilesCheckErr {type {app _}} {
  foreach l {cyl wid hei cx cy cz oframedef
    oframehang oframevang oframex oframey oframez} {
    upvar #0 $l$app $l
  }
  set rc 0
  set mchecks 1
  switch $cyl {
      cylinder {checkMv $hei height rc}
      cuboid {
	checkMv $hei height rc
	checkMv $wid width rc
      }
    default {set mchecks 0}
  }
  if {$type == "ine"} {
    if {$oframedef == "user defined frame"} {
      checkMv $oframehang "horizontal angle" rc
      checkMv $oframevang "vertical angle" rc
      checkMv $oframex "X'" rc
      checkMv $oframey "Y'" rc
      checkMv $oframez "Z'" rc
    }
  } elseif {$mchecks} {
    checkMv $cx "x direction" rc
    checkMv $cy "y direction" rc
    checkMv $cz "z direction" rc
  }
  return $rc
}

proc powCheckErr {{app _}} {
  return [samplefilesCheckErr pow $app]
}

### sample
###   san file description (SANS)
set sanESET [concat $samASET {
  {Scattering header}
  {sob radio spheres {
    "scattering\nobject" "specifies the shape of the scattering object. According to this selection, the next three parameters are taken. For
spheres: radius
ellipsoid: three radii
parallelepiped: length, width, height
cylinder: radius 1, radius 2, height
isotropic scattering: no value needed."}
    {spheres ellipsoid parallelepiped cylinder "isotropic scattering"}
    {S E P C I}
  }
  {}
  {hsrad float "" {"radius 1 or\nlength [Ang]"} gt0}
  {sobv2 float "" {"radius 2 or\nthickness [Ang]"} gt0}
  {sobv3 float "" {"radius 3 or\nheight [Ang]"} gt0}
  {}
  {rho1 float "" {"scat. len. dens.\nparticl. [1/cm²]" "scattering length density of the soluted particles"} gt0}
  {rho2 float "" {"scat. len. dens.\nsolvent [1/cm²]" "scattering length density of the solvent"} gt0}
  {fpkl float "" {"vol. fraction\nof particles" "volume fraction of the ensemble of particles in solution"} gt0}
  {}
  {miscs float "" {"incoh. scatter.\ncoeff. [1/cm]"} ge0}
  {mtscs float "" {"total scatter.\ncoeff. [1/cm]"} ge0}
  {mabcs float "" {"absorption\ncoeff. [1/cm/A]"} ge0}
  {}
}]

proc sanCheckErr {{app _}} {
  return [samplefilesCheckErr san $app]
}


proc sampleCheckErr {{app _}} {
  foreach l {bdtheta dtheta bphi bdphi} {
    upvar #0 $l$app $l
  }
  if {$bdtheta != "" || $dtheta != "" || $bphi != "" || $bdphi != ""} {
    if {$bdtheta == "" || $dtheta == "" || $bphi == "" || $bdphi == ""} {
      showText "!Either specify all of (Theta,dTheta,Phi,dPhi) or none."
      return 1
    }
    if {$bdtheta + $dtheta > 180 || $bdtheta - $dtheta < 0} {
      showText "!Please specify Theta+dTheta <= 180 and Theta-dTheta >= 0."
      return 1
    }
  }
  return 0
}

### sample
###   powder
set sample_powderESET [concat $sampleASET {
  {samplefile pareditablefile psample.par {
    "sample file" "The sample file describes the geometry and compositions of the sample. This option is mandatory." "" S} r pow 1}
}]

proc sample_powderCheckErr {{app _}} {
  return [sampleCheckErr $app]
}

### sample
###   SANS

set sample_sansESET [concat $sampleASET {
  {samplefile pareditablefile sphere.san {
    "sample file" "The sample file describes the geometry and compositions of the sample. This option is mandatory." "" S} r san 1}
}]

proc sample_sansCheckErr {{app _}} {
  if [sampleCheckErr $app] {return 1}
  return [checkMiMaErr minq maxq Q-interval $app]
}

### sample
###   S(Q)
set sample_s_qESET [concat $sampleASET {
  {samplefile pareditablefile psample.par {
    "sample file" "The sample file describes the geometry and compositions of the sample. This option is mandatory." "" S} r psq 1}
}]

proc sample_s_qCheckErr {{app _}} {
  return [sampleCheckErr $app]
}


### sample
###   singcryst

set sample_singcrystESET {
  {parfile pareditablefile sample_singcryst.par {
    "parameter file" "" "" P} r ssc 1}
  {sfactfile pareditablefile singcryst_structuref.dat {
    "structure\nfactor file" "" "" S} r}
  {spac radio Lorentzian {"d-spacing\ndistribution" "d-spacing probability distribution with maximum at the nominal value" "" o}
    {Lorentzian Gaussian} {1 2}}
  {spread float 0.0001 {"d-spacing\nspread [-]" "FWHM/d-spacing, the relative 'thickness' of the Ewald sphere" "" d}}
}

### sample->singcryst
###   ssc file description

set sscESET {
  {"Single Crystal Sample Parameter File" header}
  {ax float "" {"recipr. unit\nv. A X [1/A]" "x component of the reciprocal unit vector, A defined in the sample frame"}}
  {ay float "" {"recipr. unit\nv. A Y [1/A]" "y component of the reciprocal unit vector, A defined in the sample frame"}}
  {az float "" {"recipr. unit\nv. A Z [1/A]" "z component of the reciprocal unit vector, A defined in the sample frame"}}
  {bx float "" {"recipr. unit\nv. B X [1/A]" "x component of the reciprocal unit vector, B defined in the sample frame"}}
  {by float "" {"recipr. unit\nv. B Y [1/A]" "y component of the reciprocal unit vector, B defined in the sample frame"}}
  {bz float "" {"recipr. unit\nv. B Z [1/A]" "z component of the reciprocal unit vector, B defined in the sample frame"}}
  {cx float "" {"recipr. unit\nv. C X [1/A]" "x component of the reciprocal unit vector, C defined in the sample frame"}}
  {cy float "" {"recipr. unit\nv. C Y [1/A]" "y component of the reciprocal unit vector, C defined in the sample frame"}}
  {cz float "" {"recipr. unit\nv. C Z [1/A]" "z component of the reciprocal unit vector, C defined in the sample frame"}}
  {norm float "" {"normalisation\n[a.u]" "normalisation factor containing for example the number density"}}
  {absorb float "" {"absorption\n[1/cm/A]" "attenuation due to absorption within the sample per wavelength"}}
  {}
  {px float "" {"position X [cm]" "x coordinate of the sample centre"}}
  {py float "" {"position Y [cm]" "y coordinate of the sample centre"}}
  {pz float "" {"position Z [cm]" "z coordinate of the sample centre"}}
  {phi float "" {"phi(Z) [deg]" "rotation angle of sample i.e. the reciprocal unit vectors around the  Z axis"}}
  {chi float "" {"chi(X) [deg]" "rotation angle of sample i.e. the reciprocal unit vectors around the  X axis"}}
  {omega float "" {"omega(Z) [deg]" "rotation angle of sample i.e. the reciprocal unit vectors around the Z axis again"}}
  {}
  {geom radio cubic {geometry "sample geometry"}
    {cubic cylindrical ball} {cub cyl bal}}
  {}
  {thick float "" {"thickness\nor diameter [cm]" "rectangular sample dimension in x direction (sample frame)"}}
  {hei float "" {"height [cm]" "rectangular sample dimension in y direction (sample frame)"}}
  {wid float "" {"width [cm]" "rectangular sample dimension in Z direction (sample frame)"}}
  {oh float "" {"output angle\nhorizontal [deg]" "a frame rotation about the Z axis and then a rotation about the (new)Y axis defines a new orientation for the neutrons written to the output"}}
  {ov float "" {"output angle\nvertical [deg]"}}
}


### sample
###   inelast

set sample_inelastESET {
  {"inelastic scattering" header}
  {parfile pareditablefile sampleinelast_default.ine {
    "parameter file" "" "" P} r ine 1}
  {"S(q,w):  main (P) and dispersion (D) parameters" header}
  {p1 float 0 {"P1 [microeV]" "peak position" "" a} 1}
  {p2 float 5 {"P2 [microeV]" "constant corresponding to the half width at half maximum in the symmetrized Lorentzian" "" b} ge0 "" 1}
  {p3 float 1 {P3 "scales the amplitude, intensity" "" c} gt0 "" 1}
  {p4 float 0   {P4 "if 0, then Lorentzian, if 1, then symmetrized Lorentzian, if negative other then generates sum of 2 Lorentzians" "" d} 1}
  {temp float 1 {"temperature [K]" "temperature of the sample" "" T} gt0 "" 1}
  {reprate int 1 {repetition "repetitions for the same trajectory" "" A} ge1 "" 1}
  {d1 float 0 {"D1 [microeV*A]" "linear energy dispersion coefficient of momentum component x" "" x} 1}
  {d2 float 0 {"D2 [microeV*A]" "linear energy dispersion coefficient of momentum component y" "" y} 1}
  {d3 float 0 {"D3 [microeV*A]" "linear energy dispersion coefficient of momentum component z" "" z} 1}
  {bfact radio no {"Bose factor?" "asks whether to multiply with the Bose factor" "" D} {no yes} {0 1}}
}

### sample->ineleast
###   ine file description

set ineESET {
  {"Scattering parameters" header}
  {lf float 6.27 {"lambda\nfinal [A]" "the center of the interval of the generated outgoing random wavelength"} gt0}
  {ahf float 0 {"angle horiz\nfinal [deg]" "angle component of the main scattering direction"}}
  {avf float 0 {"angle vert\nfinal [deg]" "angle component of the main scattering direction"}}
  {dlf float 1 {"delta lambda\nfinal [A]" "the length of the interval of the generated outgoing random wavelength"}}
  {dah float 1 {"delta angle\nhoriz [deg]" "angular interval around the main scattering direction"}}
  {dav float 1 {"delta angle\nvert [deg]" "angular interval around the main scattering direction"}}
  {scc float 0.368 {"scattering\ncoeff.[1/cm]"} gt0}
  {asc float 0.109 {"absorption\ncoeff.[1/cm/A]"} gt0}
  {"Position and direction of the sample" header}
  {x1 float 50 {"X [cm]" "position of the sample centre"}}
  {y1 float 0 {"Y [cm]" "position of the sample centre"}}
  {z1 float 0 {"Z [cm]" "position of the sample centre"}}
  {hoff float 0 {"offset angle\nhoriz. [deg]" "rotation of the sample in horizontal (first rotation) direction"}}
  {voff float 0 {"offset angle\nvert. [deg]" "rotation of the sample in vertical direction"}}
  {}
  {cyl radio cylinder {"sample\ngeometry"}
    {cylinder hollow-cylinder sphere rectangular} {cyl holcyl ball cub}}
  {}
  {trad float 3 {"thickness or\ndiameter [cm]" "thickness of the sample in x direction or diameter in case of cylinder or sphere"} gt0}
  {hei float 3 {"height [cm]" "heigtht of sample in z direction if rectangular or cylinder, no relevance if sphere"} gt0}
  {wid float 3 {"inner diameter\nor width [cm]" "inner diameter of hollow cylinder or width of sample - inactiv for full cylinder option"} gt0}
  {"Output Frame" header}
  {gen radio "standard frame generation" {"output frame\ndefinition"
    "either user defined frame by using new coordinates of the output frame; or standard frame using the initial wavevector values"}
    {"standard frame generation" "user defined frame"}}
  {}
  {lai float 6.27 {"lambda\ninitial[A]*" "components of the initial main wavevector, absolute value converted to lambda"} gt0}
  {ahi float 0 {"angle horiz\ninitial[deg]*"}}
  {avi float 0 {"angle vert\ninitial[deg]*"}}
  {x2 float 50 {"X' [cm]" "position of the output frame in the original frame"}}
  {y2 float 0 {"Y' [cm]" "position of the output frame in the original frame"}}
  {z2 float 0 {"Z' [cm]" "position of the output frame in the original frame"}}
  {ha float 0 {"horiz. angle\n[deg]" "rotation angle of the output frame in horizontal (first rotation) direction (0, 0 means parallel to original X i.e. beam"}}
  {va float 0 {"vert. angle\n[deg]" "rotation angle of the output frame in vertical direction (0, 0 means parallel to original X i.e. beam"}}
}


### sample_reflectom
###

set sample_reflectomESET {
  {samref radio sample {mode "Select between sample (reflectivity data from the 'reflectivity file') and reference (reflectivity R=1 for all angles)" "" O} {sample reference} {1 2}}
  {parfile pareditablefile "" {
    "parameter\nfile" "File that contains various sample data" "" P} w ref}
  {refile parbrowsefile "" {"reflectivity\nfile"
    "File that contains the reflectivity of the sample as a function of momentum transfer.
First column: momentum transfer [1/A]\nSecond column: reflectivity" "" I} r dat}
  {axis radio Y {"axis of\nrotation" "Axis around which the sample is rotated." "" R} {Y Z}}
  {}
  {refl float 1 {"reflection\nangle \[deg\]" "the sample is rotated by this angle around the 'axis of rotation'.
zero means: parallel to x-axis,i.e. surface normal in z-direction; \n(small) positive angles cause flight directions after reflection with positive y or z components resp." "" a} -180 180}
}

proc sample_reflectomCheckErr {{app _}} {
  if {[entryVal mode $app] == "reference"} {
    if {![parFileReadable refile $app]} {
      showText "!Please specify a reflectivity file in the parameter directory"
      return 1
    }
  }
  if {![parFileReadable parfile $app]} {
    showText "!Please specify a parameter file in the parameter directory"
    return 1
  }
  upvar #0 step$app s
  upvar #0 minrefl$app mi
  upvar #0 maxrefl$app ma
  if {$s * ($ma - $mi) < 0} {
    showText "!Step size and (MaxRefl - MinRefl) must have the same sign."
    return 1
  }

  return 0
}


### sample
###   elasticisotr
set sample_elasticisotrESET {
  {pf pareditablefile sampleelastizotr_default.iso {"parameter\nfile" "" "" P} r iso 1}
  {r int 1 {repetition "" "" A}}
}

### iso file description

set isoESET {
  {"Scattering parameters" header}
  {ah float 0 {"angle horiz.\nfinal [deg]" "horizontal component of the main scattering direction"}}
  {av float 0 {"angle vert.\nfinal [deg]"  "vertical component of the main scattering direction"}}
  {}
  {dh float 1 {"delta angle\nhoriz. [deg]" "angular interval around the main scattering direction"}}
  {dv float 1 {"delta angle\nvert. [deg]"   "angular interval around the main scattering direction"}}
  {}
  {sco float 0.368 {"scattering\ncoeff.[1/cm]" "scattering cross section multiplied with density in 1/cm units"} gt0}
  {aco float 0.109 {"absorption\ncoeff.[1/cm/A]" "wavelength dependent absorption cross section multiplied with density in 1/cm/Angstrom units"} gt0}
  {"Position and direction of the sample" header}
  {x float 50 {"X [cm]" "x position of the sample center"}}
  {y float 0 {"Y [cm]" "y position of the sample center"}}
  {z float 0 {"Z [cm]" "z position of the sample center"}}
  {}
  {oh float 0 {"offset angle\nhoriz. [deg]" "rotation of the sample in horizontal (first rotation) direction"}}
  {ov float 0 {"offset angle\nvert.  [deg]" "rotation of the sample in vertical (first rotation) direction"}}
  {}
  {sg radio cylinder
    {"sample\ngeometry"}
    {cylinder hollow-cylinder sphere cuboid} {y o a u}}
  {}
  {thrad float 3 {"thickness or\ndiameter [cm]" "thickness or diameter of sample"} gt0}
  {hei float 3 {"height [cm]" "height of sample"} gt0}
  {wid float 3 {"inner diameter\nor width [cm]" "inner diameter of hollow cylinder or width of sample - inactiv for full cylinder option"} gt0}
  {"Output Frame" header}
  {x2 float 0 {"X' [cm]"}}
  {y2 float 0 {"Y' [cm]"}}
  {z2 float 0 {"Z' [cm]"}}
  {ha float 0 {"horiz.\nangle [deg]"}}
  {va float 0 {"vert.\nangle [deg]"}}
}

### eval
###   elast
set eval_elastESET {
  {psel radio "d-spacing [A]" {
    "evaluation\nparameter" "choose the parameter your interested in for your evaluation" "" k} {"d-spacing [A]" "momentum transfer Q [1/A]" "scattering angle [deg]" "wavelength difference [A]"} {1 2 3 4}}
  {}
  {sfile moneditablefile elast.eva {
    "spectra\nfile" "the spectra file: it contains the scattering results" "" o} "" "" 1}
  {ifile pareditablefile "" {
    "intensity\nfile" "intensity file (optional, see help manual) it contains the integrated intensities with respect to certain ranges of the scattering results (e.g. one is interested in the total intensity within each peak of a powder spectrum ). The ranges of integration have to be defined in the info file" "" O}}
  {infofile pareditablefile "" {
    "info\nfile" "info file: it is needed if one wants to generate an intensity file!\nThe info file then has to be defined before running this module simply as a data file as follows: each integration range is defined with two numbers. The first number gives the center of the integration interval (this corresponds e.g. to a powder peak), the second number defines the width of the integration interval.\nFor each integration range a new line must be used.\nExample:\n0.1   0.2\n0.3   0.1\nFirst line means: integration between 0.0 and 0.2, the result of the integration\nrefers to the interval center 0.1.\nSecond line means integration between 0.25 and 0.35, the result of the integration refers to the interval center 0.3." "" I}}
  {nbins int 100 {
    "number\nof bins" "number of bins determines the segmentation of the d-spacing, Q, or scatt. angle interval and therewith the number of values written to the spectra file" "" n} 1 10000}
  {mina float 0 {
    "minimum\n[A, 1/A, deg]" "lower bound of the evaluation interval" "" m} 1}
  {maxa float 0 {
    "maximum\n[A, 1/A, deg]" "upper bound of the evaluation interval" "" M} 1}
  {prob_w radio yes {
    "probability\nweight" "probability weight: the neutron probability weights, e.g. mirroring the flux distribution of the source or the sample scattering processes, can be fixed to 1 for every neutron with \"no\"" "" p} {yes no} {1 0}}
  {bin_prz float "" {
    "increase to\n next bin[%]" "case of logarithmic binning\nnumber of bins is neglected in this case" "" R} gt0}
  {dspot float "" {
    "dead-spot\n[deg]" "dead-spot: only needed if the direct beam points to the detector (as in the case of SANS).\nAll neutrons with a scattering angle(2 theta) between 0 and dead-spot will therefore not be considered in the evaluation." "" d} 0 90}
  {}
  {tof radio no {
    "time of\nflight" "(de-)activates time of flight analysis" "" w}  {yes no} {1 0}}
  {}
  {fpath float "" {
    "flight\npath [cm]" "length of total neutron flight path, needed only for time of flight analysis" "" l} gt0}
  {toff float 0 {
    "time offset [ms]" "global shift of the neutron time t t-TimeOffset [ms], useful to shift the temporal reference point for the time of flight analysis" "" T}}
  {refwave float "" {
    "reference\nwavelength [A]" "needed only for non-time of flight case, i.e. a crystal monochromator or velocity selector was used. In this case one must know which wavelength is assumed for the evaluation (to mirror the resolution adequately, naturally the stored wavelenghts cannot be used)" "" r} gt0}
  {timevalbegin float -1.e10 {
    "time interval\nbegin [ms]" "begin of time interval to be evaluated" "" e}}
  {timevalend float 1.e10 {
    "time interval\nend [ms]" "end of time interval to be evaluated" "" E}}
  {eval_colour int 0 {
    "colour" "colour necessary for the trajectory to be evaluated\ncolour 0 means: all trajectories are evaluated" "" C} 0 32768}
}

proc eval_elastCheckErr {{app _}} {
  foreach l {tof fpath toff refwave bin_prz nbins}  {
    upvar #0 $l$app $l
  }
  set rc 0
  if {$nbins == "" && $bin_prz == ""} {
    showText "!Please specify either the number of bins, or give a value for increasing to the next bin."
    set rc 1
  }
  if {$tof == "yes"} {
    if {$fpath == "" || $toff == ""} {
      showText "!Please specify flight path and time offset"
      set rc 1
    }
  } elseif {$refwave == ""} {
    showText "!Please specify reference wavelength"
    set rc 1
  }
  if [checkMiMaErr mina maxa "" $app] {
    set rc 1
  }
  return $rc
}

### eval
###   inelast

set eval_inelastESET {
  {tofile moneditablefile tofsp.eva {"TOF\nspectrum file" "Filename for the TOF spectrum datafile." "" E}}
  {efile moneditablefile energysp.eva {"energy\nspectrum file" "Filename for the energy spectrum datafile." "" G}}
  {diroinv radio "direct geometry" {
    geometry "Choose geometry type of TOF instrument." "" A}
    {"direct geometry" "inverted geometry"} {0 1}}
  {}
  {rwlen float 6.27  {"reference\nwavelength [A]" "Initial or final wavelength of the neutrons which is known from the experimental setup." "" c} gt0 "" 1}
  {pfpath float 10 {"primary flight\npath [cm]"   "Distance from the moderator to sample and from sample to detector." "" a} gt0 "" 1}
  {sfpath float 2 {"secondary flight\npath [cm]" "Distance from the moderator to sample and from sample to detector." "" b} gt0 "" 1}
  {nbins int 100 {"number\nof bins" "The number of time and energy channels to be considered." "" C} ge1 "" 1}
  {mint float 0   {"minimal\ntime [ms]" "The TOF range in which the user is interested to bin intensities." "" e} 1}
  {maxt float 100   {"maximal\ntime [ms]" "The TOF range in which the user is interested to bin intensities." "" g} 1}
  {toff float 0    {"time offset [ms]" "If nonzero, start time at moderator is shifted: TOF' = TOF - time offset." "" d} 1}
  {grtbin float 0 {"gradient\nof timebins" "Derivative s of the time channel width in function of TOF (as described in the help manual, sec. 4)." "" h} gt-0.1 lt0.1}
  {}
  {angdeg float 0  {"angle [deg]" "The user can select those neutrons which cross a smaller area on the detector surface by giving the angular position ('angle' relative to the X-axis) and width ('angle range') of a window in horizontal direction. In vertical direction no restriction is possible." "" j} 1}
  {angran float 180 {"angle\nrange [deg]" "(see angle description)" "" k} gt0 "" 1}
  {temp float 300   {"temperature [K]" "Temperature according to the temperature of the sample (only used for the case 'divide by Bose factor')." "" i} gt0}
  {divbos radio no {"divide by\nBose Factor?" "Choose whether the energy spectrum shall be normalised or not by the Bose Factor." "" D} {yes no} {1 0}}
}

proc eval_inelastCheckErr {{app _}} {
  return [checkMiMaErr mint maxt "" $app]
}

### collimator
###
set collimator_sollerESET {
  {"averaged soller collimation" header}
  {angcoll radio no {
    "angular collimation" "If angular collimation is activated certain angles will define a collimation channel. If deactivated (default) the collimxation always refers to the angle 0, i.e. the ideal flight direction along the x-axis refers to divergence 0" "" k}
     {yes no} {1 0}}
  {}
  {colldiv float "" {
    "collimator\ndivergence [deg]" "allowed x-y divergence (FWHM of the triangular shape)" "" d} ge0 "" 1}
  {eff float 0.85 {
    "peak\ntransmission" "Maximal probability for passing through the soller collimator (corresponds to an average loss due to blocking neutrons by the finite size of soller collimator spacers)" "" e} 0 1 1}
  {"case of angular collimation" header}
  {minang float 0 {
    "minimum of\nangle range [deg]" "" "" m}}
  {ncent int 1 {
    "number of\ncoll. centres" "Each collimation centre is defined by an angle which corresponds to divergence 0 (x-y-plane, angle 0 corresponds to the positive x-axis direction). The first center is defined by the 'minimum of angle range', the following centers (always higher angles) are calculated by considering a gap of (2*(allowed divergence)+ angle spacing) between the centres." "" n} ge1}
  {spang float 0 {
    "angle spacing [deg]" "Additional angular distance between the collimation centres due to the size of collimator spacers" "" a} ge0}
}

proc collimator_sollerCheckErr {{app _}} {
   foreach l {angcoll minang ncent spang}  {
    upvar #0 $l$app $l
  }
  if {$angcoll == "yes" && \
	  ($minang == "" || $ncent == "" || $spang == "")} {
    showText "!Please give minimum angle, number of coll.centers, and angle spacing"
    return 1
  }
  return 0
}


### sm_ensemble
###

set sm_ensembleESET {
  {grefdat pareditablefile sm_ensemble_beamsplitter.dat {
    "geometry and\nreflect. data" "plane shapes and reflectivity data for the supermirror components" "" P}}
  {scond int 1000 {"stop at\ncollisions" "here it stops and writes out the coordinates" "" M}}
  {sdir radio X {"spin quantisation\ndirection" "direction of spin quantisation in accordance with input data (e.g. source module)" "" Q}
    {X Y Z} {0 1 2}}
  {"output frame" header}
  {x float 200 {"X' [cm]" "x coordinate in output frame" "" r}}
  {y float   0 {"Y' [cm]" "y coordinate in output frame" "" s}}
  {z float   0 {"Z' [cm]" "z coordinate in output frame" "" t}}
  {hang float 0 {"horizontal\nangle [deg]" "angular coordinate in output frame" "" h}}
  {vang float 0 {"vertical\nangle [deg]" "angular coordinate in output frame" "" v}}
  {Visualisation header}
  {visu radio "no output" {visualisation "type of visualisation" "" T}
    {"no output" "output in collision file"
     "plane XOY" "plane XOZ" "plane YOZ"} {0 1 2 3 4 5}}
  {visdev radio display {device "visual device" "" o} {display file display+file} {1 2 3}}
  {vt radio lines {type "type of visualisation" "" c} {lines points} {0 1}}
  {h1 float "" {hmin "minimal horizontal coordinate of visualisation window" "" w}}
  {h2 float "" {hmax "maximal horizontal coordinate of visualisation window" "" W}}
  {v1 float "" {vmin "minimal vertical coordinate of visualisation window" "" a}}
  {v2 float "" {vmax "maximal vertical coordinate of visualisation window" "" A}}
  {cutoff float "" {"cutoff\nprobability" "" "" b}}
  {}
  {cfile pareditablefile collision.dat {"collision\nfile" "name of file for collisions output if 'output in collision file' option chosen in 'visualisation'" "" C}}
}

### Tool
### Compute Chopper Phases

set computeChopperPhasesTSET {
  {rpm float "" {"rounds / min." "rotational speed of the choppers\n> 0: chopper phase increases with time\n< 0: opposite direction of rotation" "" s}}
  {npl float "" {"pulse\nlength [ms]" "length of the neutron pulse [ms]" "" p}}
  {delay float 0.0 {"center of pulse\n/ delay [ms]" "average starting time of the neutrons [ms]" "" d}}
  {dsc float "" {"dist. source\nto chopper [m]" "distance source to chopper [m]" "" C}}
  {apert float "" {"chopper\naperture [deg]" "aperture of the chopper [deg]" "" a}}
  {wdeterm radio "average wavelength" {"wavelength range\ndetermined by" "" "" w}
    {"minimal wavelength" "average wavelength"} {m a}}
  {wavl float "" {"wavelength [Ang]" "minimal or average wavelength that shall pass the chopper" "" W}}
}

set computeChopperPhasesOutstring {
  wavelength range for center of pulse: #1# Ang   to #2# Ang
  wavelength range for whole pulse    : #3# Ang   to #4# Ang
  initial chopper phase               : #5# deg      #6# rotations
  chopper completely open             : #7# ms    to #8# ms
}

### Tool
### Design Chopper System

set designChopperSystemTSET {
  {reprate float "" {"repetition rate\nof pulses [1/s]" "frequency of pulses generated by the source" "" R}}
  {rpm float "" {"rounds / min." "rotational speed of the choppers\n> 0: chopper phase increases with time\n< 0: opposite direction of rotation" "" s}}
  {dsd float "" {"dist. source\n- detector [m]" "distance source to detector [m]" "" D}}
  {npl float "" {"pulse\nlength [ms]" "length of the neutron pulse [ms]" "" p}}
  {delay float 0.0 {"center of pulse\n/ delay [ms]" "average starting time of the neutrons [ms]" "" d}}
  {mpl float "" {"max. pulse\nlength [ms]" "time after beginning of pulse after which the pulse intensity is practically zero" "" m}}
  {apert float "" {"aperture WB\nchopper [deg]" "aperture of the wavelength band chopper [deg]" "" a}}
  {dsw float "" {"dist. source -\nWB chopper [m]" "distance source to wavelength band chopper [m]" "" C}}
  {bdia float "" {"beam diam. at\nWB chopper [cm]" "beam diameter and distance beam - chopper-axle of the wavelength band chopper have influence on half-shadow time and thus on usable wavelength range" "" c}}
  {dbc float "" {"dist. beam -\nchop. axle [cm]" "beam diameter and distance beam - chopper-axle of the wavelength band chopper have influence on half-shadow time and thus on usable wavelength range" "" A}}
  {dsf float "" {"dist. source -\nFO chopper [m]" "Distance source - center of frame overlap chopper [m]" "" F}}
  {bdiaf float "" {"beam diam. at\nFO chopper [cm]" "beam diameter at frame overlap chopper\n(important for half-shadow time and usable wavelength range)" "" f}}
  {wavl float "" {"wavelength [A]" "minimal or average wavelength that shall pass the chopper system" "" W}}
  {wdeterm radio "average wavelength" {"wavelength range\ndetermined by" "" "" w}
    {"minimal wavelength" "average wavelength"} {m a}}
  {fmod radio "stops no right neutron" {"FO chopper" "the frame overlap chopper apertures are chosen in a way that they\nh: do not increase the half shadow time\nn: do not stop any neutron of the right wavelength range that is able to pass the wavelength band chopper" "" k}
    {"stops no right neutron" "keeps half-shadow time"} {n h}}
  {mmod radio design {kind "no choice at the moment" "" M} {design} {o}}
}

set designChopperSystemOutstring {
  aperture FO chopper                 : #1# deg
  wavelength range for center of pulse: #2# Ang   to #3# Ang
  wavelength range for whole pulse    : #4# Ang   to #5# Ang
  initial chopper phase               : #6# deg      #7# rotations
  chopper completely open from        : #8# ms    to #9# ms
  TOF range at detector from          : #10# ms    to #11# ms
  max. evaluation time from           : #12# ms    to #13# ms
  opt. evaluation time from           : #14# ms    to #15# ms
  resp. wavelength range from         : #16# Ang   to #17# Ang
}

### Tool
### Time Distribution Plot

set distTimePlotTSET {
  {dsd float "" {"dist. source\ndetector [m]" "distance source\nto detector [m]" "" D}}
  {ttd float "" {"time to be\ndisplayed [ms]" "length of x-axis" "" t}}
  {rtp float "" {"rep. time\nof pulses [ms]" "time between two successive pulses" "" R}}
  {pl float "" {"pulse\nlength [ms]" "length of pulse" "" p}}
  {fdf int 1 {"first\ndesired frame" "frame that is displayed\nframe 1 defined by first opening of (last) chopper after beginning of pulse" "" f} ge1}
  {fname browsefile "" {"file name" "name of the file that contains the figure (with extension, without path)" "" F}}
  {tit string "" {"title of\nthe figure" "title written to the top of the figure" "" T}}
}

set b {
  {"dist. source\nto chopper [m]" ""}
  {"chopper\naperture [deg]" ""}
  {"chopper\nphase [deg]" ""}
  {"openings\nper cycle" "Number of aperture openings per cycle"}
  {"apertures\non chopper" "Number of apertures on chopper"}
}

for {set i 1} {$i <= 10} {incr i} {
  lappend distTimePlotTSET [list "Chopper $i" header]
  foreach f $b \
          n {dist cap cph opc nac} \
          t {float float float int int} \
          d {0 "" "" 1 1} \
          c {C a o N n} {
    lappend distTimePlotTSET [list ${n}_dt$i $t $d [lappend f "" $c]]
  }
}

### Help items
###
helpItem VITESS-GUI {
This graphical user interface (GUI) helps to input and edit
the parameters of a VITESS simulation.

Starting from scratch you select modules of the simulation pipe,
beginning with the first module. Upon selection the input parameters
of that module appear.
Each parameter has a <blabel> in bold characters. If you click (left mouse button)
on that label you get some lengthier explanation for that parameter
in the output window. If you need even more information you might select a module
from the Help / Modules menu. HTML text becomes shown from your browser,
which becomes started automatically.

Parameters may be input textually (of type integer, float, string ...) in the light blue
entry fields, or by selecting a radio button. Some filenames may be input
by browsing.
Without change, the parameter entries appear in the main Xcontrol window 'here'.
If you like it select the other menu entry 'separate' to edit this modules´
parameters in a separate window, or leave these parameters invisible by now
by selecting 'hidden'.
The simulation pipe happens to become longer, if you select a new module
for the last entry, which by default is the dummy entry indicated with '-----'.
If you need to insert a new module e.g. between the first and the
second module, press the button labelled with a down array left to
Module 1. If you do not need a module, press the button labeled with
a big X beneath.

In any situation you may check if your inputs are formally correct by
pressing the <bCheck>-button. The Xcontrol message window will show the
error, and your computer will beep, if something is unacceptable.
Otherwise the pipe command is to be seen.
By pressing the <bStart>-button you actually start the simulation.
The pipe is active how long it takes to complete the programs.
Stop the simulation prematurely with the <bStop>-button. A timeout
(changeable by option menu, default 28800 seconds or 8 hours)
sets an upper limit for the execution time.

To simulate an instrument with some few parameters changed you may
File / "Generate Series"
1. specify the number of iterations and which parameters to change;
   this is most easily done by clicking on labels of parameters;
2. input values; the first iteration has default values as set
   in modules; if you input a delta value and press RETURN all n-th
   values will be v1 + n*delta, v1 being the first value, n >= 2,
3. execute that series or save a tcl command file to be executed
   without GUI support.

With the <bExit>-button you terminate VITESS.

If you save all GUI settings with the 'Save Instrument' button of the
'File' title menu to a file, you may re-use these parameters later with
the 'Load Instrument' action.
'Save as Command' stores the pipe command to a text file.

1d or 2d plots of simulation results come with the 'Plot file'
and '2d Plot file' buttons of the 'File' title menu.
Monitor output files may be plotted at the end of a simulation,
if this has been chosen by a select button.

Help on modules comes with the corresponding buttons of the 'Help' menu.
The help system allows to search for keywords.
}

helpItem VITESS-General {
VITESS is a virtual instrumentation tool for neutron scattering at pulsed and
continuous sources. Please have a look on our web-site
www.hmi.de/projects/ess/vitess for more and updated information.

VITESS is supported by a graphical user interface (GUI) which generates and
controls command lines according to the given input.

A simulation comprises one ore more modules co-working sequentially
in a so called 'pipe':
-  Each module passes its neutron data to the following one, without storing
   the intermediate results to a file.
-  The first module should be a neutron source module, or read an old output file.
-  The last module should be initialised to generate an output file,
   if simulation results are not shown otherwise.

If you are interested in intermediate results without disturbing the pipe
stream you can insert the modules <bmonitor> or <bwriteout> (see help descriptions).

The GUI can control up to 30 modules. Nevertheless it is recommended for the
sake of clarity to work with less than 10 modules per pipe. A second pipe
which continues e.g. a very detailed instrument description) can then simply
be initiated by taking the output file from the preceding one as an input.

This GUI allows to enter, edit, and check the various parameters of
VITESS modules. You may save these parameters to a text file and load them
later from that file. Upon entering the parameters, you may start the command
under the control of the VITESS GUI, or generate a batch (or Tcl) command file
to be executed in a command shell.

---------------------   behind the GUI-curtain...-----------------------------
The pipe command looks as follows (the experienced user might tune this
directly without using the GUI):

module1 --f<inputfilename> -a<value> ... | module2 -a<value> ... |
	moduleN -a<value> ... --F<outputfilename>

The 'pipe' ('|') command is part of the command shell and is common in Unix
and Windows NT/95/98 systems.
(For Windows9x systems please refer to the hints given in the install.txt file.)
In effect the above example couples the standard output stream 'stdout'
of module1 with the standard input stream 'stdin' of module2. The same
is true for subsequent modules. Finally the last one, moduleN, writes
its data to the file "outputfilename".
If you are interested in intermediate results you can use the module <bwriteout>,
which stores and passes through the data it receives.

Several command line options are common for all modules in the program
package VITESS, i.e they have a common meaning. These options are

			<boption>		<bdefault>
1. neutron input filename	--f<filename>	stdin
2. neutron output filename	--F<filename>	stdout
3. rng init			--Z<value>	1
4. neutron buffer size	--B<value>	10000
5. logfilename		--L<filename>	stderr
6. dotter			--J
7. gravity 			--G		1
8. min. neutron weight	--U		1.0e-6
9. parameter directory	--P

1. neutron input filename (--f<filename>)
  This option is necessary for the first VITESS module of a pipe,
  if it is not a source module!
  <filename> denotes the file from which the VITESS module will read
  the incoming neutrons in batches of the neutron buffer size, as indicated
  by the -B option (e.g. 10000 neutrons). The file <filename> must have been
  written earlier by another VITESS module.
  In principle you are free to choose the filename <filename>, but for the sake
  of simplification one should keep a common extension of the filename, e.g. '.dat'.

2. neutron output filename (--F<filename>)
  This option is used to write the neutron data to a file. This option is only
  relevant for the last module of a pipe.
  <filename> denotes the file to which the processed neutron data are written in
  batches of the neutron buffer size. For filename restrictions see point 1.

3. rng init (--Z<value>)
  The -Z option initialises the random number generator (rng) of the VITESS modules.
  A portable random number generator is used, as described in the standard reference
  'Numerical Recipes'. The routine implementing this random number generator is
  called ran3(). <value> has to be a positive integer number.

4. neutron buffer size (--B<value>)
  The -B<value> option determines the size of the neutron input and output buffers
  of the VITESS module, in terms of the 'Neutron' structure storing the vital
  information of a neutron, i.e. position, direction, wavelength etc.. The 'Neutron'
  structure consists of nine 'double' variables and is on most computer architectures
  72 bytes large. The optimal value of the neutron buffer size depends on the computer
  the VITESS modules runs on, but 10000 is a good choice for a range of computers.

5. logfilename (--L<filename>)
  The GUI automatically generates temporary log-files, which are displayed in
  the Xcontrol message window in correct sequence after a pipe is finished
  providing some usable control information.
  Running VITESS without the GUI a logfile can be generated for each module by
  the command option -L<filename>. Omitting this option means that the
  information is written to 'stderr'.

6. dotter (--J)
  Only useful for operating without GUI: If -J appears in the command line of
  a module dots are written to the logfile (or stderr) giving an impression of
  the computation speed (one dot corresponds to the size of the neutron
  output buffer).

9. parameter directory (--P)
  Some modules need to know where they may read and store files not stated with
  parameters. The natural place for these files is the parameter directory.

The specific input parameters for each module must not be controlled
by the general command options.
}

helpItem XControl {
eXtend your eXperiment control with Xcontrol !

Xcontrol is a generic graphical user interface to control experiments.

First Xcontrol was adopted to the NEAT neutron scattering experiment,
developed at HMI department I/DN.

Contact: fromme@hmi.de
}

helpItem External-Commands {
The VITESS command pipe may contain external commands resp. programs.

These programs must read neutron data from standard input and write
resulting neutron data to standard output.

The external command may be supplied with (argc, argv)-style options,
which are specified by option string. This option string is given
as the contents of a text file.

Neutron data have the following C structure type:

typedef struct {
  double 	Time;
  double        Wavelength;
  double        Probability;
  VectorType    Position;
  VectorType    Vector;
  VectorType    Spin;
} Neutron;

}


### default editing window
###
proc editDefaults {} {
  set w .x.defaults
  generateToplevel $w "VITESS Defaults"
  fGroup $w.defaults $w.b
  generateEntries $w.defaults xcontrolDefaultsESET
  bButton $w.b.done Done "destroy $w"    puts "calling convert2Code $il1"

  pack $w.b.done
}


proc cleanupModView {} {
  global Amf
  catch {removeSubwindows $Amf}
}


### checkModVar
###
proc checkModVar {i {wishedmode ""}} {

  global DummyEntry Amf Mlf bgColor VisibleModule
  set VisibleModule $i
  set w $Mlf.g$i
  set varName mod$i
  upvar #0 $varName var
  upvar #0 visM$i visible
  upvar #0 separate$i sep
  upvar #0 separateW$i sepw
  if {$wishedmode != ""} {
    set sep $wishedmode
  }
  set wm $Mlf.m$i
  set delist {}
  if {$var == $visible} {
    if {$sep == 1 && $sepw != ""} return
    if {$sep == 0 && $sepw == "" && [winfo exists $wm.$var]} return
  } elseif {$visible != $DummyEntry} {
    # delete global entry variables of the module which is withdrawn
    set r _$i\$
    foreach n [info globals] {
      if [regexp $r $n] {
	lappend delist $n
      }
    }
  }
  set needMoreModules 0
  cleanupModView
  switch $sep {
    here {				# normal entries in main window
      catch {destroy $sepw}
      set sepw ""
      set wm $Amf;			# actual module frame
      if {$visible == $DummyEntry} {set n $wm.label} else {
	set n $wm.$visible}
      catch {destroy $n}
      set visible $var
      if {$var == $DummyEntry} {
	helpFrame $wm
      } else {
	fGroup $wm.h $wm.$var
	label $wm.h.head -text "Module $i $var" -font [headerFont] -bg $bgColor
	pack $wm.h.head
	generateEntries $wm.$var ${var}ESET $delist _$i
	set needMoreModules 1
	regsub {.c.f$} $Amf .c sw
	$sw xview moveto 0;		# scroll to canvas left
	$sw yview moveto 0;		# scroll to canvas top
      }
    }
    separate {
      set sepw $Mlf.w$i
      if [winfo exists $wm.$visible] {
	destroy $wm.$visible
      } else {
	catch {destroy $sepw}
      }
      set visible $var
      if {$var == $DummyEntry} {
	set sepw ""
      } else {
	set name "$var module $i"
	if {[getSystem] == "unix"} {set name "Vitess module $i $var"}
	generateToplevel $sepw $name
	frame $sepw.$var
	pack $sepw.$var
	set ww [giveRoom $sepw.$var BigFrame$var]
	fGroup $ww.e $ww.b
	generateEntries $ww.e ${var}ESET $delist _$i
	bButton $ww.b.done Done "destroy $sepw"
	pack $ww.b.done
	set needMoreModules 1
      }
      catch {emptyFrame $wm}
      helpFrame $Amf
    }
    hidden {
      catch {destroy $sepw}
      set sepw ""
      catch {destroy $wm.$visible}
      catch {emptyFrame $wm}
      foreach l $delist {
	global $l
	unset $l
      }
      set needMoreModules 1
    }
  }

  if {!$needMoreModules} return
  set nexti [expr $i + 1]
  if [winfo exists $Mlf.g$nexti.label] return
  moduleMenus $nexti;			# if next module menu is invisible
}

proc serializeChpFile {f mode var app} {
  set alist {nwindows radius distance hdistance}
  set blist {
    winpos0 winheight0 width0 ldeviation0 rdeviation0
    winpos1 winheight1 width1 ldeviation1 rdeviation1
    winpos2 winheight2 width2 ldeviation2 rdeviation2
    winpos3 winheight3 width3 ldeviation3 rdeviation3
  }
  foreach l [set nlist [concat $alist $blist]] {
    upvar #0 $l$app $l
  }
  if {$mode == "r"} {
    foreach l $nlist {catch {unset $l}}
    if {$f == "0"} return
    if {[nextNumItems $f 4 ni] < 3} return
    set nwindows [expr int([set n [lindex $ni 0]])]
    set radius [lindex $ni 1]
    set distance [lindex $ni 2]
    set hdistance [lindex $ni 3]
    if {$n > 4} {set n 4}
    for {set i 0} {$i < $n} {incr i} {
      if {[nextNumItems $f 5 ni] < 5} return
      foreach li $ni it {winpos winheight width ldeviation rdeviation} {
	set $it$i $li
      }
    }
  } else {
    puts $f "$nwindows\n$radius\n$distance $hdistance"
    for {set i 0} {$i < $nwindows} {incr i} {
      set c [entryVal winpos$i $app]
      foreach it {winheight width ldeviation rdeviation} {
	append c " [entryVal $it$i $app]"
      }
      puts $f $c
    }
  }
}

proc serializeCrsFile {f mode var app} {
  set alist {mposx mposy mposz offahoriz offavert bragghoriz braggvert thick width height
    dspacing reford mrange drange oframedef}
  set blist {oframex oframey oframez oframehang oframevang}
  foreach l [set nlist [concat $alist $blist]] {
    upvar #0 $l$app $l
  }
  if {$mode == "r"} {
    foreach l $nlist {catch {unset $l}}
    if {$f == "0"} return
    readNumItems $f $alist $app
    if {int($oframedef) == 0} {
      set oframedef "standard frame generation"
    } else {
      set oframedef "user defined frame"
      readNumItems $f $blist $app
    }
  } else {
    if {$oframedef == "user defined frame"} {set odef 1} else {set odef 0}
    puts $f "$mposx $mposy $mposz\n$offahoriz $offavert"
    puts $f "$bragghoriz $braggvert\n$thick $width $height\n$dspacing $reford"
    puts $f "$mrange $drange\n$odef\n$oframex $oframey $oframez\n$oframehang $oframevang"
  }
}

proc serializeSampleFile {f mode var app submodule} {
  set nlist {x y z cyl hei thick cx cy cz wid hsrad tscat cscat
      absorp vol sfac sfactfile sob sobv2 sobv3 rho1 rho2 fpkl miscs mtscs mabcs}
  foreach l $nlist {
    upvar #0 $l$app $l
  }
  if {$mode == "r"} {
    foreach l $nlist {catch {unset $l}}
    if {$f == "0"} return
    if {[gets $f line] < 0 || [gets $f l1] < 0 || \
	    [gets $f l2] < 0} return
    scan $line "%g%g%g" x y z
    switch [string range [string tolower $l1] 0 2] {
      cyl {
	set cyl cylinder
	scan $l2 "%g%g" thick hei
	if {[gets $f l2] < 0} return
	scan $l2 "%g%g%g" cx cy cz
      }
      bal {
	set cyl sphere
	scan $l2 "%g" thick
      }
      default {
	set cyl cuboid
	scan $l2 "%g%g%g" thick hei wid
	if {[gets $f l2] < 0} return
	scan $l2 "%g%g%g" cx cy cz
      }
    }
    if {[gets $f l1] < 0} return
    switch $submodule {
      san {
	catch {scan $l1 "%s%g%g%g" s hsrad sobv2 sobv3}
	switch $s {
	  S {set sob spheres}
	  E {set sob ellipsoid}
	  P {set sob parallelepiped}
	  C {set sob cylinder}
	  default {set sob "isotropic scattering"}
	}
	if {[gets $f l1] < 0 || [gets $f l2] < 0} return
	scan $l1 "%g%g%g" rho1 rho2 fpkl
	scan $l2 "%g%g%g" miscs mtscs mabcs
      }
      pow {
	set sfactfile $l1
	if {[gets $f l1] < 0 || [gets $f l2] < 0} return
	scan $l1 "%g%g%g" tscat cscat absorp
	scan $l2 "%g" vol
      }
      psq {
	if {$l1 == "D"} {set sfac "from file"} else {set sfac "as function"}
	if {[gets $f l1] < 0 || [gets $f l2] < 0} return
	set sfactfile $l1
	scan $l2 "%g%g%g" tscat cscat absorp
      }
      default {	scan $l1 "%g%g%g%g" tscat cscat absorp vol}
    }
  } else {
    puts $f "$x $y $z"
    switch $cyl {
      cylinder {puts $f "cyl\n$thick $hei\n$cx $cy $cz"}
      sphere {puts $f "bal\n$thick"}
      default {puts $f "cub\n$thick $hei $wid\n$cx $cy $cz"}
    }
    switch $submodule {
      san {
	switch $sob {
	  spheres {set s S}
	  ellipsoid {set s E}
	  parallelepiped {set s P}
	  cylinder {set s C}
	  default {set s I}
	}
	puts $f "$s $hsrad $sobv2 $sobv3\n$rho1 $rho2 $fpkl\n$miscs $mtscs $mabcs"
      }
      pow {puts $f "$sfactfile\n$tscat $cscat $absorp\n$vol"}
      psq {
	if {$sfac == "from file"} {set s D} else {set s F}
	puts $f "$s\n$sfactfile\n$tscat $cscat $absorp"
      }
      default {	puts $f "$tscat $cscat $absorp $vol"}
    }
  }
}

proc serializeSanFile {f mode var app} {
  serializeSampleFile $f $mode $var $app san
}

proc serializePowFile {f mode var app} {
  serializeSampleFile $f $mode $var $app pow
}

proc serializePsqFile {f mode var app} {
  serializeSampleFile $f $mode $var $app psq
}

proc serializePolFile {f mode var app} {
  set nlist {dx dy dz nc dw gx gy gz ax ay az}
  foreach l $nlist {
    upvar #0 $l$app $l
  }
  if {$mode == "r"} {
    foreach l $nlist {catch {unset $l}}
    if {$f == "0"} return
    readNumItems $f $nlist $app
  } else {
    puts $f "$dx $dy $dz\n$nc $dw\n$gx $gy $gz\n$ax $ay $az"
  }
}

proc serializeIsoFile {f mode var app} {
  set alist {ah av dh dv sco aco x y z oh ov}
  set blist {thrad hei wid x2 y2 z2 ha va}
  foreach l [set nlist [concat $alist sg $blist]] {
    upvar #0 $l$app $l
  }
  if {$mode == "r"} {
    foreach l $nlist {catch {unset $l}}
    if {$f == "0"} return
    if [readNumItems $f $alist $app] {
      if {[gets $f l1] >= 0} {
	switch [string index [string tolower $l1] 1] {
	  y {set sg cylinder}
	  o {set sg hollow-cylinder}
	  a {set sg sphere}
	  default {set sg cuboid}
	}
      }
      readNumItems $f $blist $app
    }
  } else {
    puts $f "$ah $av\n$dh $dv\n$sco $aco\n$x $y $z\n$oh $ov"
    switch $sg {
      cylinder {puts $f cylinder}
      hollow-cylinder {puts $f holcyl}
      sphere   {puts $f ball}
      default  {puts $f cuboid}
    }
    puts $f "$thrad $hei $wid\n$x2 $y2 $z2\n$ha $va"
  }
}

proc serializeIneFile {f mode var app} {
  set alist {lf ahf avf dlf dah dav scc asc x1 y1 z1 hoff voff}
  set blist {trad hei wid gen lai ahi avi x2 y2 z2 ha va}
  foreach l [set nlist [concat $alist cyl $blist]] {
    upvar #0 $l$app $l
  }
  if {$mode == "r"} {
    foreach l $nlist {catch {unset $l}}
    if {$f == "0"} return
    if [readNumItems $f $alist $app] {
      if {[gets $f l1] >= 0} {
	set fitem [string tolower [lindex [split $l1] 0]]
	switch [string range $fitem 0 2] {
	  cyl {set cyl cylinder}
	  hol {set cyl hollow-cylinder}
	  bal {set cyl sphere}
	  default {set cyl rectangular}
	}
	if [readNumItems $f $blist $app] {
	  if {$gen} {set gen "user defined frame"} else {
	    set gen "standard frame generation"}
	}
      }
    }
  } else {
    puts $f "$lf $ahf $avf\n$dlf $dah $dav\n$scc $asc\n$x1 $y1 $z1\n$hoff $voff"
    switch $cyl {
      cylinder {puts $f cyl}
	  hollow-cylinder {puts $f holcyl}
      sphere   {puts $f bal}
      default  {puts $f cub}
    }
    if {$gen == "user defined frame"} {set ggen 1} else {set ggen 0}
    puts $f "$trad $hei $wid\n$ggen\n$lai $ahi $avi\n$x2 $y2 $z2\n$ha $va"
  }
}

proc serializeSscFile {f mode var app} {
  set alist {ax ay az bx by bz cx cy cz norm absorb px py pz phi chi omega}
  set blist {geom}
  set clist {thick wid hei oh ov}
  foreach l [set nlist [concat $alist $blist $clist]] {
    upvar #0 $l$app $l
  }
  if {$mode == "r"} {
    foreach l $nlist {catch {unset $l}}
    if {$f == "0"} return
    if [readNumItems $f $alist $app] {
      if {[gets $f line] < 0} return
      switch [string range [string tolower $line] 0 2] {
	cyl {set geom cylindrical}
	cub {set geom cubic}
	default {set geom ball}
      }
      readNumItems $f $clist $app
    }
  } else {
    puts $f "$ax $ay $az\n$bx $by $bz\n$cx $cy $cz\n$norm $absorb\n$px $py $pz\n$phi $chi $omega"
    switch $geom {
      cubic {puts $f cub}
      ball  {puts $f bal}
      default {puts $f cyl}
    }
    puts $f "$thick $wid $hei\n$oh $ov"
  }
}

proc serializeRefFile {f mode var app} {
  set alist {mx my mz thick wid hei gen}
  set blist {horang vertang x y z}
  foreach l [set nlist [concat $alist $blist]] {
    upvar #0 $l$app $l
  }
  if {$mode == "r"} {
    foreach l $nlist {catch {unset $l}}
    if {$f == "0"} return
    if [readNumItems $f $alist $app] {
      if $gen {
	# user frame
	set gen "user defined frame"
	readNumItems $f $blist $app
      } else {
	set gen "standard defined frame"
      }
    }
  } else {
    puts $f "$mx $my $mz\n$thick $wid $hei\n"
    if {$gen == "user defined frame"} {
      puts $f "1\n$horang\n$vertang\n$x $y $z"
    } else {
      puts $f 0
    }
  }
}

proc prepList {l n} {
  set a {}
  foreach e $l {
    lappend a $e$n
  }
  return $a
}

proc convert2String {ll} {
  # convert radio codes to string
  set modl {- "decoupled poisoned" "decoupled unpoisoned" coupled multi-spectral}
  set l {}
  set i -1
  foreach e $ll {
    if {[incr i] == 2} {
      # shape on position 2, must be C or R
      if {$e == 1 || $e == "C"} {
	lappend l circular
      } else {
	lappend l rectangular
      }
    } elseif {$i >= 11 && $i <=13} {
      # filenames on positions 11 12 13, may be none
      if {$e == "none" || $e == 0} {
	lappend l {}
      } else {
	lappend l $e
      }
    } elseif {$i == 14} {
      # modtype on position 14, must be from 0 to 4
      if {$e < 0 || $e > 4} {set e 0}
      lappend l [lindex $modl $e]
    } else {
      lappend l $e
    }
  }
  return $l
}

proc convert2Code {ll app} {
  set s ""
  set i -1
  foreach e $ll {
    set v [entryVal $e $app]
    if {[incr i] == 2} {
      if {$v == "circular"} {set v C} else {set v R}
    } elseif {$i >= 11 && $i <= 13} {
      if {$v == "" || $v == "0"} {set v none}
    } elseif {$i == 14} {
      # be careful: v might have - as value
      switch -- $v {
	"decoupled poisoned" {set v 1}
	"decoupled unpoisoned" {set v 2}
	coupled {set v 3}
	multi-spectral {set v 4}
	default {set v 0}
      }
    } elseif {$i == 17} {
      switch -- $v {
	TS1 {set v 1}
	TS2 {set v 2}
	default {set v 0}
      }
    } elseif {$v == ""} {
      set v 0
    }
    append s "$v "
  }
  return $s
}

proc serializeModFile {f mode var app} {
  set al {temp color shape cx cy cz width height spaord scale current
    wfile tfile wtfile modtype tau1 tau2}
  set il1 [prepList $al 1]
  set il2 [prepList $al 2]

  set nlist [concat $il1 $il2]
  upvar #0 usemod2$app umod

  if {$mode == "r"} {
    foreach l $nlist {
      upvar #0 $l$app $l
      catch {unset $l}
    }
    set umod unused
    if {$f == "0"} return
    set imode -1
    while {[gets $f line] >= 0} {
      set ll [itemize $line]
      set fi [lindex $ll 0]
      if {$fi == ""} continue
      if {[string index $fi 0] == "#"} continue
      switch [incr imode] {
	0 {set tl $il1}
	1 {set tl $il2
	  set umod used
	}
      }
      if {[llength $ll] != [llength $tl]} continue
      set ll [convert2String $ll]
      foreach item $tl i $ll {
	set r $i
	catch {eval "set $item \$r"}
      }
    }
  } else {
    foreach l $nlist {
      # supply dummy values for items without meaning for cws/lpss sources
      upvar #0 $l$app $l
      if {[info exist $l] == 0} {set $l 0}
    }
    puts $f "# Source
# Moderators:  center size  distribution files  time
# Temp. col shape x y z wid|dia hei spaord tot_flux curr w-file t-file wt-file  Mod tau_a tau_d"
    puts $f [convert2Code $il1 $app]
    if {$umod == "used"} {
      puts $f [convert2Code $il2 $app]
    }
  }
}

proc serializeCmoFile {f mode var app} {
  serializeModFile $f $mode $var $app
}
proc serializeLmoFile {f mode var app} {
  serializeModFile $f $mode $var $app
}
proc serializeSmoFile {f mode var app} {
  serializeModFile $f $mode $var $app
}

proc serializeImoFile {f mode var app} {
  set il {cx cy cz width wtfile tstat}
  set al {temp color shape cx cy cz width height spaord scale current
    wfile tfile wtfile modtype tau1 tau2 tstat}

  if {$mode == "r"} {
    foreach l $il {
      upvar #0 $l$app $l
      catch {unset $l}
    }

    if {$f == "0"} return
    set imode -1
    while {[gets $f line] >= 0} {
      set ll [itemize $line]
      set fi [lindex $ll 0]
      if {$fi == ""} continue
      if {[string index $fi 0] == "#"} continue
      set ll [convert2String $ll]
      set cx [lindex $ll 3]
      set cy [lindex $ll 4]
      set cz [lindex $ll 5]
      set width [lindex $ll 6]
      set wtfile [lindex $ll 13]
      switch [lindex $ll 17] {
	2 {set tstat TS2}
	default {set tstat TS1}
      }
    }
  } else {
    foreach l $al {
      # supply dummy values for items without meaning for cws/lpss sources
      upvar #0 $l$app $l
      if {[info exist $l] == 0} {set $l 0}
    }
    puts $f "# Source
# Moderators:  center size  distribution files  time
# Temp. col shape x y z wid|dia hei spaord tot_flux curr w-file t-file wt-file  Mod tau_a tau_d ISIS"
    puts $f [convert2Code $al $app]
  }
}


proc editSave {var param ext app {saveAs 0}} {
# param = 1 forces that a file with new filename is within
# the given default directory
  if [errorWithValues $ext 1 $app] return
  if {$ext == "chp" && [chpCheckErr $app]} return
  upvar #0 $var$app v
  if $saveAs {set v [fileDialog write $ext]}
  catch {set resfn $v}
  if $param  {forceParamDir v resfn}

  if [catch {open $resfn w} f] {
    showText "can't open $resfn to write"
    return
  }
  set w .fedit$var
  if {[set p [getSerializeProc $ext]] != ""} {
    if [catch {$p $f w $var $app} res] {
      showText "!Some error occured on writing $resfn ($res)"
    } else {
      showText "Successfully wrote $resfn"
    }
  } else {
    puts $f [$w.v.text get 1.0 end]
  }
  close $f
  destroy $w
}


proc editFile {var param ext app} {
  # Edit parameters of a module, which are separated in
  #  a parameter file with extension ext.
  #  $var$app is the name of the parameter file.
  #  param is a parameter for editSave.
  # This GUI generator relies on serialize${ee}File
  #  (where $ee is capitalized $ext) to read a file
  #  and editSave to store the results.

  upvar #0 $var$app v
  if {![info exists v] || $v == ""} {
    showText "!Please enter a filename first!"
    return
  }
  set w .fedit$var
  catch {destroy $w}
  if {$param != 0} {
    forceParamDir v resfn
  } else {
    set resfn $v
  }
  generateToplevel $w "Edit $v"
  fGroup $w.v $w.b
  if {[set serializeproc [getSerializeProc $ext]] == ""} {
    global bgColor
    text $w.v.text -relief raised -bd 2 \
	-height 32 -width 80\
	-font [monoFont] -bg $bgColor\
	-setgrid 1\
	-yscrollcommand "$w.v.yscroll set"
    yscroll $w.v "$w.v.text yview"
    pack $w.v.text -side left -fill both -expand yes
  }

  if [catch {open $resfn r} f] {
    showText "old file $resfn did not exist"
    set f 0
  }

  if {$serializeproc != ""} {
    catch {$serializeproc $f r $var $app}
    set ww [giveRoom $w.v BigFrame$ext]
    generateEntries $ww ${ext}ESET {} $app
  } elseif {$f != "0"} {
    while {[gets $f line] >= 0} {$w.v.text insert end "$line\n"}
  }
  if  {$f != "0"} {close $f}

  bButton $w.b.check Check "clearText; errorWithValues $ext 1 $app"
  bButton $w.b.save "Save+Close" "editSave $var $param $ext $app"
  bButton $w.b.saveas "Save As" "editSave $var $param $ext $app 1"
  bButton $w.b.cancel Cancel "destroy $w"
  pack $w.b.check $w.b.save $w.b.saveas $w.b.cancel -side left -expand 1
}

proc helpOnModule {i} {
  global Htmlhelp
  upvar #0 mod$i m
  if [catch {set h $Htmlhelp($m)}] return
  showHelpItem $h
}

proc trimModules {w i rmlist deflist} {
  deleteSomeModules $w $i
  # delete all entry variable settings of superseeded modules
  foreach l $rmlist {
    global $l
    unset $l
  }
  # redefine saved entry variables for shifted module
  foreach item $deflist {
    global [set gvar [lindex $item 0]]
    set $gvar [lindex $item 1]
  }
  # reactivate saved modules for new indices
  reShowModules $w
}

proc moveDown {oldi} {
  global maxModule DummyEntry maxModule Mlf
  # check if there is some free room below
  for {set i $oldi} {$i <= $maxModule} {incr i} {
    upvar #0 visM$i vv
    if {![info exists vv] || $vv == "" || $vv == $DummyEntry} break
  }
  if {$i > $maxModule} return
  set newi $oldi
  set rmlist  {}
  set deflist {}
  lappend deflist [list visM$oldi $DummyEntry] [list mod$oldi $DummyEntry]
  set allglob [info globals]
  set w $Mlf;

  # append a free module below
  if {$i < $maxModule} {        # else we're full
    set lasti 1
    for {set i $oldi} {$i <= $maxModule} {incr i} {
      upvar #0 visM$i visible
      if [info exists visible] continue
      incr i -1
      break
    }
    if {[globVal mod$i] != $DummyEntry} {
      incr i
      if {![winfo exists $w.g$i.label]} {
	moduleMenus $i
      }
    }
  }

  # save module names of active modules with index ge $oldi
  for {set i $oldi} {$i <= $maxModule} {incr i} {
    incr newi
    upvar #0 visM$i visible
    if {![info exists visible] || $newi > $maxModule} break
    lappend deflist [list visM$newi $visible]
    lappend deflist [list mod$newi [globVal mod$i]]
    lappend rmlist visM$i mod$i
    set r _$i\$
    foreach n $allglob {
      if [regexp $r $n] {
	lappend rmlist $n
	regsub $r $n _$newi newr
	# save entry variable settings of these modules
	lappend deflist [list $newr [globVal $n]]
      }
    }
  }
  trimModules $w $oldi $rmlist $deflist
}


proc removeMod {oldi} {
  global maxModule DummyEntry Mlf
  set newi $oldi
  set rmlist  {}
  set deflist {}
  set allglob [info globals]
  set w $Mlf
  set remains 0
  cleanupModView
  # save module names of active modules with index ge $oldi + 1
  for {set i $oldi} {$i <= $maxModule} {incr i} {
    upvar #0 visM$i visible
    if {![info exists visible]} break
    lappend rmlist visM$i mod$i
    set act [globVal mod$i]
    if {$act == $DummyEntry} continue
    if {$i > $oldi} {
      set remains 1
      lappend deflist [list visM$newi $visible] [list mod$newi $act]
    }
    set r _$i\$
    foreach n $allglob {
      if [regexp $r $n] {
	lappend rmlist $n
	if {$i > $oldi} {
	  regsub $r $n _$newi newr
	  # save entry variable settings of these modules
	  lappend deflist [list $newr [globVal $n]]
	}
      }
    }
    if {$i > $oldi} {
      incr newi
    }
  }
  if {$oldi > 1 || $remains} {
    lappend deflist [list visM$newi $DummyEntry] [list mod$newi $DummyEntry]
    trimModules $w $oldi $rmlist $deflist
  }
}

### moduleMenus
###
proc moduleMenus {{n 1}} {
  global AvailableSET maxModule DummyEntry Mlf labColor radioColor menuColor menuButtonColor
  set fn [headerFont]
  set lfn [labelFont]
  set tfn [textFont]
  set maxi $maxModule
  if {$maxi > $n} {set maxi $n}

  if {![string match fdown [image names]]} {
    set fpath [file join [globVal SourceDirectory] BITMAPS]
    image create bitmap fdown -file  [file join $fpath downarr.xbm]
    image create bitmap fright -file [file join $fpath rightarr.xbm]
    image create bitmap ftop -file [file join $fpath toparr.xbm]
    image create bitmap fcross -file [file join $fpath cross.xbm]
  }

  # prepend button to digest view if a digest has been defined
  set w $Mlf.dig.f
  if {"" == [globVal digestSource]} {
    catch {destroy $w}
  } elseif {! [winfo exists $w]} {
    Frame $w
    button $w.cross -image fcross -command removeDigest
    button $w.right -image fright -command digestView
    label $w.l -text "Instrument Digest"\
	-font $lfn -bg $menuButtonColor
    pack $w.cross -side left  -anchor w
    pack $w.right -side right -padx 1 -anchor w
    pack $w.l -side top -fill x -anchor w
  }

  for {set i 1} {$i <= $maxi} {incr i} {
    set w $Mlf.g$i
    set cm "checkModVar $i"
    if [winfo exists $w.label] continue
    if {$i <= 9} {set ti "  $i"} else {set ti $i}
    label $w.label -text $ti -font $fn -bg $labColor
    bind $w.label <ButtonPress> "helpOnModule $i"
    upvar #0 separateW$i sepw
    set sepw ""
    upvar #0 separate$i sepvar
    set sepvar here

    if {$i < $maxModule} {set c "moveDown $i"} else {set c ""}
    button $w.down -image fdown -command $c
    button $w.right -image fright -command "checkModVar $i here"
    button $w.top -image ftop -command "checkModVar $i separate"
    button $w.cross -image fcross -command "removeMod $i"

    set varName mod$i
    upvar #0 $varName var
    set var $DummyEntry
    upvar #0 visM$i visible
    set visible $var
    set wm $w.opt.menu
    menubutton $w.opt -textvariable $varName -indicatoron 1 \
	-menu $wm -font $lfn -relief raised -bd 2 -width 18 \
	-highlightthickness 2 -anchor c -bg $menuButtonColor
    menu $wm -tearoff 0 -bg $menuColor
    $wm add radiobutton -label $DummyEntry -variable $varName \
	-command $cm -font $lfn
    foreach label $AvailableSET {
      set j [lindex $label 0]
      if {[set subl [lindex $label 1]] == ""} {
	$wm add radiobutton -label $j -variable $varName \
	    -command $cm -font $lfn
      } else {
	set subm $wm.$j
	$wm add cascade -label $j -menu $subm -font $lfn
	menu $subm -tearoff 0
	foreach jj $subl {
	  $subm add radiobutton -label $jj -variable $varName \
	      -command $cm -font $lfn
	}
      }
    }
    pack $w.cross $w.down $w.label $w.opt $w.top $w.right -side left -padx 1 -anchor w
  }
}

# Unset temporary help variables to prevent them from being saved.
# (variables matching single characters, or with Add in the end are
# deleleted by setAll)

set TempVars {m0 m1 m2 m3 Mod1 Mod2 Refa Refb Refc Refm
    dA nA nnA mA pA tA i1 i2 ll li al sps fr fl}
foreach n $TempVars {
  catch {unset $n}
}

# unset tool routines
foreach p {sore genFE genFE2} {
  proc $p {} {}
}

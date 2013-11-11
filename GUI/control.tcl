### project Xcontrol
### HMI DN
### M. Fromme fromme@hmi.de
### June 1999

### main module for experiment control
###
### not experiment specific
###
proc setAll {{mode 0}} {
  global globalDescriptionSET
  foreach nn $globalDescriptionSET {
    setGlobals $mode $nn
  }
  unset nn
  # delete all temp. variables with a name matching
  #   single letter
  #   ..ASET
  #   ..Add
  foreach ee [info globals] {
    if [regexp {^.$|ASET$|Add$} $ee] {
      global $ee
      unset $ee
    }
  }
}

proc finalExit {} {
  global tcl_platform FilesToDeleteList
  if {$tcl_platform(os) == "Darwin"} {destroy .}
  if [info exists FilesToDeleteList] {
    foreach f $FilesToDeleteList {
      catch {file delete $f}
    }
  }
  set fdir [file join [globVal SourceDirectory] FILES .saved]
  foreach f [glob -nocomplain -directory $fdir *.gui] {
    catch {file delete $f}
  }
  exit
}

### catch destroy events from window manager, but
### normal exit at confirmed situations
###
proc windowManagerExit {} {
  global KillMe
  if {[info exists KillMe] && $KillMe} exit
  if [dontDoit "Exit VITESS\nchanges not saved yet"] {
    # restore withdrawn last chance window
    wm deiconify .
  } else {
    finalCheck
    finalExit
  }
}

proc finalCheck {} {
  for {set i 3} {$i < 10} {incr i} {
    if [catch {tell file$i}] continue
    puts "file $i is still open"
  }
}

proc confirmedExit {} {
  global TryingToExit KillMe
  if {[info exists KillMe] && $KillMe} return
  set nt [clock seconds]
  if [catch {set ov $TryingToExit}] {
    set TryingToExit $nt
  } elseif {$nt - $ov < 5} {
    return
  }
  if [dontDoit "Exit VITESS\nchanges not saved yet"] return
  set KillMe 1
  catch {closeCmdHandles}
  finalCheck
  finalExit
}

proc showModulesAgain {{delall 0}} {

  global Mlf XRoot mod1 DummyEntry instrumentfile LastWin
  # save name of first module and instrument name,
  # and re-set them after destruction/construction from scratch
  set savmod1 [globVal mod1]
  set savname $instrumentfile

  if $delall {
    # do not terminate if the hook is destroyed in foreach loop
    bind $LastWin <Destroy> {}
    foreach w [winfo children $XRoot] {
      destroy $w
    }
  }
  showBeef $XRoot
  if {$savmod1 != "" && $savmod1 != $DummyEntry} {
    set mod1 $savmod1
  }
  reShowModules $Mlf
  setInstrumentfile $savname
  if {[getSystem] == "unix"} {
    # Give a hint of the overall geometry, otherwise we see a stamp
    # sized window with Linux. We may not specify the exact size, sorry;
    # In conjunction with KDE the result is 
    # "maximize to full window vertically".
    wm geometry $XRoot 800x600
  }
}

proc applySettings {} {
  showModulesAgain 1
}

# possible sizes of truetype fonts, first elements are default
# for for displays > 1024x780,
# second used for windows per default, third smallest possible
set HFontSizes {12 9 7}
set BFontSizes {12 9 7}
set LFontSizes {11 8 6}
set TFontSizes {10 8 6}

set FontSizeMinIndex 0
# check the maximal window size
if {[winfo screenwidth .] <= 1024 ||
    [winfo screenheight .] <= 780 ||
    [getSystem] == "windows"} {
  set FontSizeMinIndex 1
}
set FontSizeIndex $FontSizeMinIndex

proc setFontSizes {} {
  global HFontSizes BFontSizes LFontSizes TFontSizes
  global FontSizeIndex hfontsize lfontsize bfontsize tfontsize mfontsize monofontsize
  set hfontsize [lindex $HFontSizes $FontSizeIndex]
  set lfontsize [lindex $LFontSizes $FontSizeIndex]
  set bfontsize [lindex $BFontSizes $FontSizeIndex]
  set tfontsize [lindex $TFontSizes $FontSizeIndex]
  set mfontsize $bfontsize
  set monofontsize $tfontsize
}

proc smallerFonts {} {
  global FontSizeIndex
  if {$FontSizeIndex >= 2} return
  incr FontSizeIndex
  setFontSizes
  showModulesAgain 1
}

proc biggerFonts {} {
  global FontSizeIndex FontSizeMinIndex
  if {$FontSizeIndex <= $FontSizeMinIndex} return
  incr FontSizeIndex -1
  setFontSizes
  showModulesAgain 1
}

proc chooseColor {{mode 1}} {
  global bgColor labColor radioColor menuButtonColor canvasColor\
      menuColor buttonColor entryColor
  switch $mode {
    1 {set c $bgColor; set b background}
    2 {set c $buttonColor; set b button}
    3 {set c $entryColor; set b entry}
  }
  set c [tk_chooseColor -initialcolor $c -title "Choose $b color"]
  if {$c == ""} return
  switch $mode {
    1 {
      set bgColor $c
      set labColor $c
      set radioColor $c
      set menuButtonColor $c
      set canvasColor $c
      set menuColor $c
    }
    2 {set buttonColor $c}
    3 {set entryColor $c}
  }
  applySettings
}


proc copyModPars {} {
  upvar #0 VisibleModule vi
  upvar #0 visM$vi visible
  global DummyEntry CopiedPars CopiedValues
  if {$visible == $DummyEntry} return
  set cp {}; set cv {}
  foreach n [info globals] {
    if [regexp (.+)_$vi\$ $n a pa] {
      lappend cp $pa
      lappend cv [globVal $n]
    }
  }
  set CopiedPars($visible) $cp
  set CopiedValues($visible) $cv
}

proc pasteModPars {} {
  upvar #0 VisibleModule vi
  upvar #0 visM$vi visible
  global CopiedPars CopiedValues
  if [catch {set cp $CopiedPars($visible); set cv $CopiedValues($visible)}] return

  foreach p $cp cvi $cv {
    gSet ${p}_$vi $cvi
  }
}



# mMenue $w.fil ... produces $w.fil.menu
###
proc controlMenu {w} {
  global neededModulesSET menuColor
  global AvailableSET SourceDirectory Htmlhelp tcl_platform

  mMenu $w.fil File
  mMenu $w.copa Edit
  mMenu $w.plo Plot
  mMenu $w.con Configure
  mMenu $w.opt Options
  mMenu $w.tool Tools
  mMenu $w.hel Help
  pack $w.fil $w.copa $w.plo $w.con $w.tool $w.opt $w.hel -side left -ipadx 2m

  set lmenu {
    {c "LOAD Instrument" {loadAll gui}}
    {c "SAVE Instrument" {storeAll gui}}
    {c "SAVE As" {storeAll gui newfile.gui}} s
    {c "ADD Packet" {addPacket}}
    {c "INSERT Packet" {insertPacketWindow}}
    {c "SAVE Packet" {savePacketWindow}} s
    {c "SAVE to Directory" saveDirectory} s
    {c "Import Pipe" {importPipe}}
    {m "Export as" mex} s
    {c "Generate Series" {genSeries .gser}} s
    {c "Merge Results" {mergeRes .mres}} s
    {c "New *.inf File" editInfFile}
    {c "Edit *.inf File" {editInfFile 1}} s
  }
  lappend lmenu {c EXIT confirmedExit}
  eval popMenu $w.fil.menu $lmenu

  menu $w.fil.menu.mex -bg $menuColor -tearoff 0
  set flist {
    {c "tcl script" {storeAll tcl}}
    {c "pl perl script" {storeAll pl}}
    {c "py python script" {storeAll py}}
  }
  if {[getSystem] == "unix"} {
    lappend flist \
      {c "sh shell script" {storeAll sh}} \
      {c "sh grid script" {storeAll grd}}
  } else {
    lappend flist \
      {c "bat shell script" {storeAll bat}}
  }
  eval popMenu $w.fil.menu.mex $flist
  unset flist

  popMenu $w.copa.menu \
      {c "Recover Instrument" recoverFileGUI} \
      {s} \
      {c "Copy  Module Parameters" copyModPars} \
      {c "Paste Module Parameters" pasteModPars}

  set lmenu {{c "Plot File" {plotFile 1}} {c "2D Plot File" {plotFile 2}}}
  if {"" != [getGnuPlotApp]} {
    lappend lmenu \
        {c "Plot Cmd" {plotCmdWindow}}\
        {c "Plot using Template" {plotTemplateCmdWindow}} s\
        {c "Close gnuplot Windows" {closeCmdHandles}} s\
        {c "New Template" {newTemplate}}\
        {c "Edit Template" {editTemplate}}
  }
  eval popMenu $w.plo.menu $lmenu


  popMenu $w.con.menu \
      {c "Set Instrument Name" setInstrumentName} s\
      {c "Define Instrument Digest" genDigest}

  set clist {ascii2bin
    define_direction direct_view gener_batch mirror_coating surface_file gener_bispectral
    standard_deviation rvitess lattice_dist guide_shape
  }
  set htmlist $clist
  lappend htmlist crysanalyzerspec chop_phases chop_phases dist_time
  lappend clist "cas_v40+--Z2 --LCAS_log60arm10.dat -PCAS_par60arm10.dat -SCAS_S60arm10.dat -TCAS_D60arm10.dat -l6.174745 -w0.03 -k0" chop_phases:computeChopperPhases chop_phases:designChopperSystem dist_time:distTimePlot:1

  set nlist {"Convert Ascii to Binary"
    "Define Direction"
    "Direct View" "Generate Batches" "Generate Mirror Files" "Generate Surface Files" "Generate Extraction System"
    "Standard Deviation" "Read and Visualise Output"
    "Lattice Distances" "Guide Shape"
    "Cryst. Analyzer Spectrom."
    "Compute Chopper Phases" "Design Chopper System"
    "Distance Time Plot"
  }

  foreach n $nlist com $clist {
    if {[string first : $com] >= 0} {
      set c doGUICommand
      foreach l [split $com :] {lappend c $l}
      lappend clist [list c $n $c]
    } elseif {[set si [string first + $com]] >= 0} {
      set comapp [string range $com [expr $si + 1] end]
      set com [string range $com 0 [expr $si - 1]]
      lappend clist [list c $n "doToolCommand $com \"$comapp\""]
    } else {
      lappend clist [list c $n "doToolCommand $com"]
    }
  }
  eval popMenu $w.tool.menu $clist

  popMenu $w.hel.menu \
      {c "General information" {showHelpItem VITESS-General}} \
      {c Tutorial {showHelpItem tutorial.pdf}} \
      {c "User interface" {showHelpItem VITESS-GUI}} \
      {c "Generate Series" {showHelpItem sim_series.html}} \
      {c "Instrument Digest" {showHelpItem digest.html}} \
      {c "External commands" {showHelpItem External-Commands}} \
      {c "Ray tracing" {showHelpItem raytracing.html}} \
      {c Trajectories {showHelpItem trajectories.html}} \
      {m Tools me} s \
      {c Xcontrol {showHelpItem XControl}} s \
      {m "Modules A - F" m1} \
      {m "Modules G - P" m2} \
      {m "Modules R - Z" m3}

  set pwd [file join $SourceDirectory WWW]

  # Add a help link for all available external commands
  set li {}
  foreach h $htmlist n $nlist {
    if [file exists [file join $pwd $h.html]] {
      set Htmlhelp($n) $h.html
      lappend li [list c $n "showHelpItem $h.html"]
    }
  }
  if {[llength $li] > 0} {
    menu $w.hel.menu.me -bg $menuColor -tearoff 0
    eval popMenu $w.hel.menu.me $li
  }

  # Add a help link for all modules with given HTML help file.
  set nl {}
  set hl {}
  foreach line $AvailableSET {
    set n [lindex $line 0]
    set s [lindex $line 1]
    set h [lindex $line 2]
    if {$s == ""} {
      lappend nl $n
      lappend hl $h
    } else {
      set fe [lindex $h 0]
      set lasthelp ""
      foreach t $s m $h {
	lappend nl $t
	if {$m == ""} {
	  set m $lasthelp
	} else {
	  set lasthelp $m
	}
	lappend hl $m
      }
    }
  }
  set li1 {}
  set li2 {}
  set li3 {}
  foreach m $nl h $hl {
    if [file exists [file join $pwd $h.html]] {
      set Htmlhelp($m) $h.html
      set ll [list c $m "showHelpItem $h.html"]
      switch -regexp $m {
	^[a-fA-F] {lappend li1 $m $ll}
	^[g-pG-P] {lappend li2 $m $ll}
	default {lappend li3 $m $ll}
      }
    }
  }
  menu $w.hel.menu.m1 -bg $menuColor -tearoff 0
  eval popMenu $w.hel.menu.m1 $li1

  menu $w.hel.menu.m2 -bg $menuColor -tearoff 0
  eval popMenu $w.hel.menu.m2 $li2

  menu $w.hel.menu.m3 -bg $menuColor -tearoff 0
  eval popMenu $w.hel.menu.m3 $li3

  set wo $w.opt.menu
  popMenu $wo \
      {c "Apply settings" applySettings} s\
      {c "Load settings" {fileSettings 0}} \
      {c "Save settings" {fileSettings 1}} s\
      {c "Smaller fonts" smallerFonts} \
      {c "Bigger fonts" biggerFonts} \
      {m Fonts afont} s\
      {m "Check mode" checkmode} \
      {m "Save Instr. mode" savemode} \
      {m "Output compression" compmode} \
      {m "Execution mode" execmode} \
      {m Buffersize buffersize} \
      {m "Plot mode" plotmode} \
      {m Trajectories trajmode} \
      {m "Browse selection" browse_ext_mode} \
      {m "Scrollbar width" swid} s\
      {m Xcontrol intern} \
      {m Recovery recovery} s\
      {c "X3D options" editX3DOptions} \
      {c "Helper applications" editDefaults}

  set ww $wo.recovery
  menu $ww -bg $menuColor -tearoff 0
  popMenu $ww \
      {m Snapshots states} \
      {m "Snap Interval (s)" secs}

  forceDef StoreStates 8
  cascEntries $ww.states StoreStates 2 4 8 16

  forceDef WatchSeconds 30
  cascEntries $ww.secs WatchSeconds 5 10 20 30 60 300 600 900

  set ww $wo.afont
  menu $ww -bg $menuColor -tearoff 0
  popMenu $ww \
      {m text tfont}\
      {m "monospaced text" monofont}\
      {m menubar mfont}\
      {m header hfont}\
      {m button bfont}\
      {m label lfont}

  fontMenu $ww mfont
  fontMenu $ww hfont
  fontMenu $ww bfont
  fontMenu $ww lfont
  fontMenu $ww monofont
  fontMenu $ww tfont

  set ww $wo.intern
  menu $ww -bg $menuColor -tearoff 0
  popMenu $ww \
      {m "GUI Style" tk_strictMotif} \
      {m Color color} s\
      {m "Info level" infolevel} \
      {m Timeout timeout} s\
      {m Bell bell} \
      {m Protocol prot}

  forceDef ProtocolMode action
  cascEntries $ww.prot ProtocolMode action everything nothing

  forceDef tk_strictMotif [expr {[getSystem] == "unix"}]
  cascEntries $ww.tk_strictMotif tk_strictMotif 1 0

  forceDef audible_bell on
  cascEntries $ww.bell audible_bell on off

  set www $ww.color
  menu $www -bg $menuColor -tearoff 0
  popMenu $www \
      {c Background {chooseColor 1}} \
      {c Buttons {chooseColor 2}} \
      {c Entries {chooseColor 3}}

  forceDef timeout unlimited
  cascEntries $ww.timeout timeout 10 100 500 1000 3600\
      5400 10000 20000 28800 57600 172800 unlimited

  forceDef Infolevel user
  cascEntries $ww.infolevel Infolevel user expert

  forceDef Checkmode normal
  cascEntries $wo.checkmode Checkmode normal set_default strict

  forceDef SaveInstrmode normal
  cascEntries $wo.savemode SaveInstrmode normal "with series"

  forceDef Execmode normal
  cascEntries $wo.execmode Execmode normal "save old" "copy results"

  forceDef plotmode dots
  cascEntries $wo.plotmode plotmode dots "dots + lines"

  # prefer X3D over SVG
  forceDef trajmode X3D
  cascEntries $wo.trajmode trajmode X3D "SVG xz" "SVG xy" textfile

  if {$tcl_platform(os) == "Darwin"} {
    # Mac Darwin won't let you select files visible, which haven't a given extension
    forceDef browse_ext_mode all
  } else {
    forceDef browse_ext_mode select
  }
  cascEntries $wo.browse_ext_mode browse_ext_mode all select

  forceDef Compmode none
  cascEntries $wo.compmode Compmode none nodebug float gzip nodebug+gzip float+gzip


  if {[getSystem] == "windows"} {set scw 16} else {set scw 8}
  forceDef scrollWidth $scw
  cascEntries $wo.swid scrollWidth 4 8 12 16

  forceDef buffersize 10000
  cascEntries $wo.buffersize buffersize 1000 5000 10000 20000 50000 100000

  focus $w
}

proc menubarFont {} {
  global mfontfamily mfontsize mfonttype
  return [list $mfontfamily $mfontsize $mfonttype]
}
proc headerFont {} {
  global hfontfamily hfontsize hfonttype
  return [list $hfontfamily $hfontsize $hfonttype]
}
proc buttonFont {} {
  global bfontfamily bfontsize bfonttype
  return [list $bfontfamily $bfontsize $bfonttype]
}
proc sbuttonFont {} {
  global bfontfamily lfontsize bfonttype
  return [list $bfontfamily $lfontsize $bfonttype]
}
proc ssbuttonFont {} {
  global tfontfamily lfontsize tfonttype
  return [list $tfontfamily [expr $lfontsize - 1]  $tfonttype]
}
proc labelFont {} {
  global lfontfamily lfontsize lfonttype
  return [list $lfontfamily $lfontsize $lfonttype]
}
proc stextFont {} {
  global tfontfamily tfontsize tfonttype
  return [list $tfontfamily [expr $tfontsize - 2] $tfonttype]
}
proc textFont {} {
  global tfontfamily tfontsize tfonttype
  return [list $tfontfamily $tfontsize $tfonttype]
}
proc monoFont {} {
  global monofontfamily monofontsize monofonttype
  return [list $monofontfamily $monofontsize $monofonttype]
}
proc bigLabelFont {{a ""}} {
  global sserif FontSizeIndex
  set ls [lindex {18 14 12} $FontSizeIndex]
  if {$a != ""} {incr ls $a}
  if {$ls >= 16 && [winfo screenwidth .] <= 1024} {set ls 12}
  return [list $sserif $ls bold]
}

proc setOptions {} {
  global serif sserif monospaced \
      mfontfamily mfonttype \
      hfontfamily hfonttype bfontfamily bfonttype \
      lfontfamily lfonttype tfontfamily tfonttype \
      monofontfamily monofontsize monofonttype
  if [info exists hfontfamily] return

  setFontSizes

  set hfontfamily $sserif
  set hfonttype bold

  set bfontfamily $sserif
  set bfonttype bold

  set lfontfamily $serif
  set lfonttype bold

  set tfontfamily $sserif
  set tfonttype normal

  set monofontfamily $monospaced
  set monofonttype $tfonttype

  set mfontfamily $bfontfamily
  set mfonttype $bfonttype

  set mf [menubarFont]
  option add *Menubutton.font $mf
  option add *Menu.font $mf
  option add *Button.font [buttonFont]
  option add *Label.font  [labelFont]
  option add *font [textFont]
  option add *monofont [monoFont]
}

proc pardirPar {} {
  global defdirectory_
  set d [file join $defdirectory_ a]
  return [string range $d 0 [expr [string length $d] - 2]]
}

proc promptCommand {prog withargs} {
  set w .diacom
  catch {destroy $w}
  generateToplevel $w "Tool Command Execution" "" "+50+300"

  fGroup $w.h1 $w.h2 $w.b
  global labColor bgColor defdirectory_

  label $w.h1.l -text "Tool Command:" -font [labelFont] -bg $labColor
  pack $w.h1.l -side left -anchor w
  text $w.h2.t -wrap word -width 100 -height 3 -bg $bgColor
  $w.h2.t insert end "$prog $withargs"
  pack $w.h2.t -side left -anchor w

  bButton $w.b.c Cancel "destroy $w"
  bButton $w.b.n Execute "startAction \[$w.h2.t get 1.0 end\] tool"
  pack $w.b.c -side left -anchor w
  pack $w.b.n -side right -anchor w
}

proc doToolCommand {prog {withargs ""}} {
  global ExeDirectory ExeSuffix Browser
  set c [file join $ExeDirectory $prog$ExeSuffix]
  if {$withargs != ""} {
    promptCommand $c $withargs
    return
  }
  if {[getSystem] == "unix"} {
    exec xterm -e $c --P[pardirPar] &
    return
  }
  regsub -all / [pardirPar] \\ p
  regsub -all / $c \\ c
  exec $Browser /EXE $c "/PARAMS:--P$p" &
}

proc performCommand {prog mod {tw ""} {ts ""}} {
  global ExeDirectory ExeSuffix
  upvar #0 FullCommand fc
  set fc [file join $ExeDirectory $prog$ExeSuffix]

  foreach l [globVal ${mod}TSET] {
    writeCommandOption $l _gt
  }

  # puts "doing :exec $fc --P[pardirPar]:"
  # execute the command, catch errors
  if [catch {eval exec $fc --P[pardirPar]} res] {
    if {$prog == "dist_time"} {
      showText $res
    } else {
      showText "!could not execute tool command $prog\n$res"
    }
    return
  }
  if {$tw == ""} return
  set i 1
  foreach s [split $res \n] {
    regsub \n $s "" s
    regsub -all "\#$i\#" $ts [string range "           $s" end-10 end] ts
    incr i
  }
  $tw delete 1.0 end
  $tw insert end $ts
}

proc doGUICommand {prog mod {big ""}} {
  global bgColor FontSizeIndex
  set w .guitool
  catch {destroy $w}
  generateToplevel $w "Tool Module $mod"
  set com [list performCommand $prog $mod]
  if {$big != ""} {
    set ew 16
    set eh 12
    if {$FontSizeIndex == 0} {
      incr ew 2
      incr eh 2
    }
    set ww [scrollFrame $w right ${ew}c ${eh}c 40c]
    fGroup $ww.b $ww.e
    bButton $ww.b.do Plot $com
    bButton $ww.b.canc Cancel [list destroy $w]
    pack $ww.b.do $ww.b.canc -side left
    generateEntries $ww.e ${mod}TSET {} _gt
    return
  }
  fGroup $w.e $w.o $w.b
  generateEntries $w.e ${mod}TSET {} _gt
  upvar #0 ${mod}Outstring o
  if [info exists o] {
    global lfontsize lfonttype
    text $w.o.t -wrap none -width 100 -height [llength [split $o \n]]\
	-bg $bgColor -font [list Courier $lfontsize $lfonttype]
    # $w.o.t insert end $o
    pack $w.o.t -fill x
    lappend com $w.o.t $o
  }
  bButton $w.b.do Calculate $com
  bButton $w.b.canc Cancel [list destroy $w]
  pack $w.b.do $w.b.canc -side left
}

proc trVar {n e op} {
  global ProgressS ProgressTextL
  # ProgressS is in the range 0-99
  if {$ProgressS == 0} {
     set ProgressTextL ""
  } else {
    set ProgressTextL "$ProgressS %"
  }
}

proc showBeef {w} {
  global bgColor canvasColor buttonColor xcontrolDefaultsESET \
      maxModule DummyEntry Mlf Amf Textw Messagew Tth sserif XRoot FontSizeIndex

  set XRoot $w
  frame $w.mbar -relief raised -bd 2 -bg $bgColor
  pack $w.mbar -side top -fill both

  # This is the place where main GUI elements are created.
  # Global setups like sizes and limits are set here.
  set t "VITESS 3.2"
  set maxModule 100
  set DummyEntry "--inactive--"

  frame $w.bm -bg $bgColor; # top header
  frame $w.h  -bg $bgColor; # heading
  pack $w.bm $w.h -side top -fill both

  set Root $w.m
  frame $Root -bg $bgColor; # frame for module list and module edit window
  set Textw $w.t
  frame $Textw -bg $bgColor;  # text window
  pack $Root $Textw -side top -fill both -expand yes

  frame $w.h.b -bg $bgColor -relief sunken -bd 2
  pack $w.h.b -side left
  frame $w.h.r -bg $bgColor -relief sunken -bd 2
  pack $w.h.r -side right
  frame $w.h.input -bg $bgColor -relief sunken -bd 2
  pack $w.h.input -fill both

  # the FontSizeIndex select the size of GUI canvas sizes
  # 0 for relative big workstation displays
  # 1,2.. smaller displays, like VGA resolution

  # pcm: pixel per cm
  set pcm [winfo fpixels . 1c]

  # hpx available whole window height in pixel
  set hpx [winfo screenheight .]

  # ch: fixed height of module list and visible module window, in cm
  # 0.8 means the VITESS window should not take more than 80 % of the display height
  # 12/28 is the ratio of module list window per total height we want to obtain

  set ch [expr $hpx * 0.8 * 12.0/28.0 / $pcm]c

  # length and width of window components given in cm
  # cw    list canvas width
  # ch    list canvas height
  # amw   actual module frame width
  # cmw   header canvas width
  # vbh   vitess banner height

  # Tth   text window height, in characters of given font, means visible text lines
  set Tth 10

  switch $FontSizeIndex {
    0 {
      set cw 9c
      set amw 19c
      set cmw 12c
      set vbh 1.0c
    }
    default {
      set cw 8c
      set amw 17c
      set cmw 11c
      set vbh 0.8c
    }
  }
  set sh ${maxModule}c;				# scrolled list virtual height
  frame $Root.l -relief sunken -bd 2
  frame $Root.r -relief sunken -bd 2
  pack $Root.l -side left -fill both
  pack $Root.r -side right -fill both -expand yes

  textWindow $Textw $Tth [textFont]
  set Messagew $Textw

  set Amf [scrollFrame $Root.r both $amw $ch $sh]; # Actual module frame
  set Mlf [scrollFrame $Root.l left $cw $ch $sh];  # Module list frame
  set com "fGroup $Mlf.dig"
  for {set i 1} {$i <= $maxModule} {incr i} {
    append com " $Mlf.g$i $Mlf.m$i"
  }
  eval $com

  moduleMenus

  controlMenu $w.mbar

  label $w.bm.hlab -bg $bgColor -fg steelblue
  setInstrumentfile 1
  bind $w.bm.hlab <ButtonPress> setInstrumentName

  set blf [bigLabelFont]
  upvar #0 MainBitmap bitm
  if {[info exists bitm] && $bitm != "" && ![catch {glob $bitm}]} {
    image create photo image1 -file $bitm
    label $w.bm.c -image image1 -bd 1 -relief sunken
    pack $w.bm.c $w.bm.hlab -padx .5m -pady .5m
  } else {
    canvas $w.bm.c -width $cmw -height $vbh -bg $canvasColor \
	-highlightbackground $canvasColor
    $w.bm.c create text 7c 5 -fill steelblue -font $blf -anchor n -text $t
    pack $w.bm.hlab $w.bm.c -side left
  }

  set blf [bigLabelFont -3]
  label $w.bm.notice -bg $bgColor -fg steelblue -font $blf -text "Click parameter names for help!"
  pack $w.bm.notice -side right
  bind $w.bm.notice <ButtonPress> {showHelpItem VITESS-GUI}

  foreach line $xcontrolDefaultsESET {
    forceDef "[lindex $line 0]_" [lindex $line 2]
  }

  helpFrame $Amf

  ### action buttons
  global fileentrywidth LastWin Progress ProgressS ProgressTextL
  set savw $fileentrywidth
  set fileentrywidth 72

  generateEntries $w.h.input inputESET
  set fileentrywidth $savw

  set wb $w.h.b
  frame $wb.check
  bsButton $wb.check.c1 Check checkAction
  bsButton $wb.check.c2 "Dryrun" startActionD
  pack $wb.check -anchor w -fill x
  pack $wb.check.c1 $wb.check.c2 -side left -ipadx 1m
  bButton $wb.start Start startAction
  bButton $wb.startv Trajectories startActionV
  frame $wb.meter
  frame $wb.stop
  bsButton $wb.stop.kill "  Kill  " "stopAction 1 1"
  bsButton $wb.stop.stop "  Stop   " stopAction
  pack $wb.start $wb.startv -fill x

  pack $wb.meter -fill x -anchor w
  set Progress [set ProgressS 0]
  if {"" == [info command ttk::progressbar]} {
    set ProgressTextL ""
    set wl $wb.meter.l
    label $wl -textvariable ProgressTextL
    pack $wl
    trace variable Progress w trVar
  } else {
    ttk::progressbar $wb.meter.progress -orient horizontal -mode determinate -variable ProgressS
    pack $wb.meter.progress -fill x
  }
  pack $wb.stop -anchor w -fill x
  pack $wb.stop.stop $wb.stop.kill -side left -ipadx 1m

  set wb $w.h.r
  bButton $wb.del Fresh deleteAllModules
  bButton $wb.exit Exit confirmedExit
  frame  $wb.dummy
  pack $wb.del -fill x
  pack $wb.dummy -fill x -anchor w -pady 12m
  pack $wb.del $wb.exit -fill x
  saveLastState
  set LastWin $wb.exit
  bind $LastWin <Destroy> windowManagerExit
}

proc saveLastState {} {
  # Save the state, which is hashed in a generated command.
  global LastState
  set LastState [generateVitessCommand kstate]
}

proc doSnapshot {} {
  global LastSnapState LastCheck StateStoreInd StoreStates

  set i 0

  set st [generateVitessCommand kstate]

  set sexist [info exists StateStoreInd]

  if {! $sexist} {
    set do_snap 1
  } elseif {! [info exists LastSnapState]} {
    set do_snap 1
  } elseif {$st == $LastSnapState} {
    set do_snap 0
  } else {
    set do_snap 1
  }

  if $do_snap {
    set LastSnapState $st

    if $sexist {
      set i [expr [incr StateStoreInd] % $StoreStates]
    } else {
      set StateStoreInd 0
    }
    set fdir [file join [globVal SourceDirectory] FILES .saved]
    file mkdir $fdir
    set fn [file join $fdir $i.gui]
    storeAll gui "" $fn 0
  }

  # indicate snap time to watchdog
  set LastCheck [clock seconds]
}

proc watchdogMonitor {} {
  global WatchSeconds PipeActive LastCheck
  
  set tsec $WatchSeconds
  if {! $PipeActive} {
    set do_snap 1
    if [info exists Lastcheck] {
      set tdelta [expr [clock seconds] - $LastCheck]
      if {$tdelta < $tsec} {
        set do_snap 0
        incr tsec -$tdelta
      }
    }
    if $do_snap doSnapshot
  }
  # reenable watchdog execution
  after [expr 1000*$tsec] watchdogMonitor
}

### main control widget of Xcontrol, specific for VITESS if the name of this widget
### is not the main wish.
### We establish . as a withdrawn "last chance" widget, which becomes
### visible only in cases of <Destroy> events, probably from
### the window manager.

proc controlGUI {
		 defaultdirectory
		 edescription
		 {w .}
		 {geo ""}
		 {restart 0}
	       } {

  global simulation ProtocolMode Protfile XcontrolVersion buttonColor sserif

  ### version control : do not load constants saved with a version number
  ### with different integer part (versions like 1.0 and 1.10 are
  ### compatible, but versions 1 and 2 are not)
  ###
  set XcontrolVersion 2.1

  if {!$restart && $w != "."} {
    ###
    ### protocol utilities :
    ### Protfile is the file variable of the protocol
    ### file (or "" if none)

    set ProtocolMode action
    set Protfile ""

    ### global simulation mode
    ###
    set simulation off

    setOptions
    wm title . "Last chance"
    wm geometry . +400+400
    set font [buttonFont]
    button .sos -text "Restore Xcontrol" \
	-command [list controlGUI $defaultdirectory $edescription $w $geo 1] \
	-font $font -bg $buttonColor
    button .exit -text Exit -command exit -font $font -bg $buttonColor
    pack .sos .exit -fill both -ipady 1m
    #pack .exit -fill both -ipady 1m
    wm withdraw .
  }

  generateToplevel $w "Xcontrol [pwd]" "" $geo

  if $restart {
    # last chance window goes to the hide again
    wm withdraw .
    showModulesAgain
  } else {
    global ControlDirectory WatchSeconds PipeActive
    set PipeActive 0
    set ControlDirectory $defaultdirectory
    setAll
    # for the xcontrol root window names start with . dot
    if {$w == "."} {set w ""}
    showBeef $w
    after [expr 1000*$WatchSeconds] watchdogMonitor
  }

}

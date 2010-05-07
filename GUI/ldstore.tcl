### project Xcontrol
### HMI DN
### M. Fromme fromme@hmi.de
### June 1999

proc reShowModules {w} {
  global maxModule DummyEntry LastWin
  cleanupModView
  set list {}
  for {set i 1} {$i <= $maxModule} {incr i} {
    set m [globVal mod$i]
    if {"" == $m} break
    lappend list $m
  }
  set ll [llength $list]
  # now find last non-dummy
  for {set i [expr [llength $list] - 1]} {$i > 0} {incr i -1} {
    if {$DummyEntry != [lindex $list $i]} break
  }
  set list [lrange $list 0 $i]
  if {$i > 0} {
    lappend list $DummyEntry
  }
  set i [llength $list]

  moduleMenus $i;			# module menus with DummyEntry strings
  set j 1
  foreach n $list {
    upvar #0 mod$j visible
    set visible $n;			# re-show the right ones
    upvar #0 separate$j sep
    set sep hidden;			# set mode hidden
    incr j
  }
}

proc removeTrailingDummies {} {
  global maxModule DummyEntry Amf
  set firsti [set lasti 0]
  for {set i 1} {$i <= $maxModule} {incr i} {
    upvar #0 mod$i v
    if {[info exists v] && $v == "$DummyEntry"} {
      if {$firsti == 0} {set firsti $i}
      set lasti $i
    }
  }
  if {$firsti != $lasti} {
    removeMod $firsti
  }
}

proc storeAll {extension {prosal ""} {as ""}} {
  conditionalOpenProtfile
  if {$extension == "gui"} {
    if {$as != ""} {
      if [catch {open $as w} f] {
	outProtocol "can't open $as to write"
	return
      }
      set fname $as
    } else {
      if {$prosal == ""} {
	set prosal [globVal instrumentfile]
	if {[file extension $prosal] == ""} {
	  append prosal ".gui"
	}
      }
      if {[set f [openWriteFile gui $prosal fname]] == 0} return
    }
    set n [file tail $fname]
    set la [string last "." $n]
    if {$la > 0} {set n [string range $n 0 [incr la -1]]}
    setInstrumentfile $n
  } else {
    if {[set f [openWriteFile $extension]] == 0} return
  }
  switch $extension {
    gui {
      puts $f "#experiment description save file"
      puts $f "#version [globVal XcontrolVersion]"
      foreach g [savableGlobals] {
	puts $f "gSet $g \{[globVal $g]\}"
      }
    }
    bat {
      set c [generateVitessCommand bat]
      if {"windows" == [getSystem]} {regsub -all / $c \\ c}
      puts $f $c
    }
    default {
      puts $f [generateVitessCommand $extension]
    }
  }
  close $f
  outProtocol "file stored"
  conditionalCloseProtfile
  gSet LastState [generateVitessCommand action]
}

proc deleteSomeModules {w i} {
  # deactivate all modules with index ge $i
  global maxModule DummyEntry
  for {} {$i <= $maxModule} {incr i} {
    removeSubwindows $w.g$i
    removeSubwindows $w.m$i
    upvar #0 separateW$i sepw
    catch {destroy $sepw}
    upvar #0 visM$i v
    upvar #0 mod$i mv
    if [info exists v] {
      set v [set mv $DummyEntry]
    }
  }
}


gSet sInameESET {
  {setiname string "" {instrument "Instrument name: This should be a meaningful name."} "" "" 1}
}

proc doSetIname {w} {
  set n [entryVal setiname]
  if {$n == ""} {
    tk_messageBox -message "Please enter an instrument name!"
    return
  }
  setInstrumentfile $n
  destroy $w
}

proc setInstrumentName {} {
  set w .siname
  catch { destroy $w}
  generateToplevel $w "Set Instrument Name" "" +300+300
  fGroup $w.v $w.b
  generateEntries $w.v sInameESET

  bButton $w.b.cancel Cancel "destroy $w"
  bButton $w.b.save Set "doSetIname $w"
  pack $w.b.cancel -side left
  pack $w.b.save -side right
}

proc setInstrumentfile {name} {
  global instrumentfile sserif
  regsub -all " " $name _ name
  set instrumentfile $name
  regexp {[0-9a-zA-ZäöüÄÖÜß_-]+} [file tail $instrumentfile] a
  if {[winfo screenwidth .] <= 1024} {set ls 12} else {set ls 16}
  .x.bm.hlab configure -text "Instrument $a" \
      -font [list $sserif $ls bold]
}


###
proc loadAll {extension} {
  if [dontDoit "You have unsaved changes. Forget them?"] return

  set name [fileDialog open $extension]
  if {$name == ""} return

  # check if file has been written by a previous storeAll
  set version -1
  if [catch {open $name r} f] { return 0}
  if {[gets $f] == "#experiment description save file"} {
    if {1 != [scan [gets $f] "#version %d" version]} {
      set version 0
    }
  }
  close $f
  if {$version == -1} {
    outProtocol "! file $name does not exists or is unusable to load"
    return 0
  }
  set a [globVal XcontrolVersion]
  if {int($version) != int($a)} {
    outProtocol "! version $version of $name doesn't match actual version $a"
    return 0
  }

  # remember old default directory
  global defdirectory_ Mlf
  set olddef $defdirectory_

  # this will probably be the new default directory
  set nd [file dirname $name]

  # delete all modules
  deleteSomeModules $Mlf 1

  # If a gui-file becomes loaded, settings for GUI sizes and the default directory
  # would be changed, too. If values come from a different OS, these values would be
  # non-sense or not applicable, at least these changes probably are unexpected.
  # So we store GUI settings before loading, and re-set them afterwards.
  set savlist {defdirectory_ maxModule scrollWidth serif sserif monospaced
    itemlabwidth fileentrywidth}
  set vallist {}
  foreach s $savlist {lappend vallist [globVal $s]}
  foreach s {b h l m t} {
    foreach t {family size type} {
      lappend vallist [globVal [set n ${s}font$t]]
      lappend savlist $n
    }
  }

  # now load all definitions by sourcing the gui-file
  if [catch {source $name} res] {
    set errs "dubious gui file $name, $res"
  } else {
    set errs "control file $name successfully loaded"
  }

  # restore old GUI settings
  foreach s $savlist sval $vallist {
    gSet $s $sval
  }

  setAll 0
  conditionalOpenProtfile
  outProtocol "--- $errs ---"
  conditionalCloseProtfile

  reShowModules $Mlf

  removeTrailingDummies
  setInstrumentfile $name

  set defdirectory_ $olddef; # restore old value
  # but ask if modified new default directory is ok
  confirmedCommand gSet "defdirectory_ $nd" "Set default directory to $nd"
  gSet LastState [generateVitessCommand action]

  return 1
}

proc deleteAllModules {} {
  # delete all modules
  global Mlf Amf
  deleteSomeModules $Mlf 1
  reShowModules $Mlf
  removeTrailingDummies
  setInstrumentfile 1
  gSet LastState ""
  helpFrame $Amf
  foreach e [stringToSet [info globals]] {
    if [regexp {^([A-Z_.]|error|auto_|arg|tk|tcl|blt_)|env|(SET|Add|Outstring)$} $e] continue
    if [regexp {_([0-9]+)$} $e] {
      global $e
      catch {unset $e}
    }
  }
}


gSet saveDirESET {
  {savedir browsedir new {"new save\ndirectory" "All parameter files used in the actual module selction, and a GUI file accorrding to this module selection, will be copied to this new directory, which will become the default directory thereafter."} w "" 1 d}
  {insname string "" {instrument "Instrument name: This should be a meaningful name."} "" "" 1}
}

proc doSaveDir {w} {
  upvar #0 defdirectory_ pdir
  set d [entryVal savedir]
  if {[file pathtype $d] != "absolute"} {
    set d [file join $pdir $d]
  }

  if [file exists $d] {
    tk_messageBox -message "file $d exists\nplease enter a new directory name!"
    return
  }
  set iname [entryVal insname]
  if {$iname == ""} {
    tk_messageBox -message "Please enter an instrument name!"
    return
  }

  # find all used parameter file names
  global DummyEntry maxModule
  set pall {}
  for {set i 1} {$i <= $maxModule} {incr i} {
    set varName mod$i
    upvar #0 $varName var
    if {![info exists var] || $var == $DummyEntry} continue
    foreach l [globVal ${var}ESET] {
      set vname [lindex $l 0]
      switch [lindex $l 1] {
	parfilename - pareditablefile - parbrowsefile - moneditablefile - mon2editablefile {
	  lappend pall [entryVal $vname _$i]
	}
      }
    }
  }
  # determine all existing used parameter file names
  set cpl {}
  foreach fname [stringToSet [join [lsort $pall]]] {
    set n [file join $pdir $fname]
    if [file readable $n] {
      lappend cpl $n
    }
  }

  if [catch {file mkdir $d} res] {
    tk_messageBox -message $res
    return
  }

  # copy parameter files to new directory
  if {[llength $cpl] > 0} {
    if [catch {eval file copy $cpl $d} res] {
      tk_messageBox -message $res
    }
  }

  set pdir $d;				# set new default directory
  set fname [file join $pdir $iname]
  setInstrumentfile [file join $pdir $iname]
  if {[file extension $fname] == ""} {
    append fname ".gui"
  }
  storeAll gui "" $fname
  outProtocol "saved instrument environment to directory $d"
  destroy $w
}

proc saveDirectory {} {
  set w .sdir
  catch {destroy $w}
  generateToplevel $w "Save to New Directory" "" +300+300
  fGroup $w.v $w.b
  generateEntries $w.v saveDirESET

  bButton $w.b.cancel Cancel "destroy $w"
  bButton $w.b.save Save "doSaveDir $w"
  pack $w.b.cancel -side left
  pack $w.b.save -side right
}

proc saveInfFile {w fn} {
  if [catch {open $fn w} f] {
    showText "!!could not rewrite instrument file $fn"
  } else {
    puts $f [$w.v.text get 1.0 end]
    close $f
    showText "Instrument file $fn written"
  }
  destroy $w
}

proc editInfFile {{mode 0}} {
  global defdirectory_ bgColor monospaced
  if $mode {set ft open} else {set ft write}
  if {[set fn [fileDialog $ft inf instrument.inf]] == 0} return
  set w .editinf
  catch {destroy $w}
  generateToplevel $w "Edit Instrument File"
  fGroup $w.v $w.b
  text $w.v.text -relief raised -bd 2 \
      -height 32 -width 150\
      -font [list $monospaced 8 normal] -bg $bgColor\
      -setgrid 1\
      -yscrollcommand "$w.v.yscroll set"
  yscroll $w.v "$w.v.text yview"
  pack $w.v.text -side left -fill both -expand yes
  if [catch {open $fn r} f] {
    showText "old file $fn did not exist"
  } else {
    while {[gets $f line] >= 0} {$w.v.text insert end "$line\n"}
    close $f
  }
  bButton $w.b.save Save+Close "saveInfFile $w $fn"
  bButton $w.b.cancel Cancel "destroy $w"
  pack $w.b.save $w.b.cancel -side left -expand 1
}



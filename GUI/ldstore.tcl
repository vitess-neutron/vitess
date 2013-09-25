### project Xcontrol
### HMI DN
### M. Fromme fromme@hmi.de
### June 1999

proc reShowModules {w} {
  global maxModule DummyEntry LastWin
  disableModule ;  # set all modules enabled
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

proc doSavePacket {w} {
  set m1 [entryVal smod1]
  set m2 [entryVal smod2]
  set as [entryVal spacketfile]
  conditionalOpenProtfile
  if {$as != ""} {
    set flist [file split $as]
    if {[llength $flist] < 2} {
      set as [file join [entryVal defdirectory] $as]
    }
    if [catch {open $as w} f] {
      outProtocol "can't open $as to write"
      return
    }
  } else {
    if {[set f [openWriteFile $extension]] == 0} return
  }
  puts $f "#packet file"
  puts $f "#version [globVal XcontrolVersion]"

  # selected exists:ge
  set resi 1
  for {set i $m1} {$i <= $m2} {incr i} {
    set v [globVal mod$i]
    if {$v != "" && $v != "--inactive--" } {
      set ge($i) $resi
      puts $f "gSet mod$resi \{$v\}"
      incr resi
    }
  }
  foreach g [savableGlobals] {
    if [regexp {^(.+)_([0-9]+)$} $g a v n] {
      if [info exists ge($n)] {
	set resi $ge($n)
	puts $f "gSet ${v}_$resi \{[globVal $g]\}"
      }
    }
  }
  close $f
  outProtocol "packet file stored ($as)"
  conditionalCloseProtfile
  catch {destroy $w}
}


proc activeModules {} {
  # which modules are active?
  # return a list of module numbers; at least 1 is "active"
  set m [globVal maxModule]
  set n {}
  for {set i 1} {$i < $m} {incr i} {
    set s [globVal mod$i]
    if {$s == "" || $s == "--inactive--"} break
    lappend n $i
  }
  return $n
}

proc savePacketWindow {} {
  set actm [activeModules]
  if {[llength $actm] <= 0} return
  set w .spacket
  catch {destroy $w}
  generateToplevel $w "Save packet"
  fGroup $w.v $w.b

  set l1 [list smod1 radio 1 {"first module"} $actm $actm]
  set l2 [list smod2 radio [lindex $actm end] {"last module"} $actm $actm]

  set dname [file join [entryVal defdirectory] packet.gui]
  gSet savePacketESET [list $l1 $l2 [list spacketfile parbrowsefile $dname {
    "package\nfilename" "The package definitions will be stored to a .gui file."} w gui]]
  generateEntries $w.v savePacketESET

  bButton $w.b.cancel Cancel "destroy $w"
  bButton $w.b.save "Save Packet" "doSavePacket $w"
  pack $w.b.cancel -side left
  pack $w.b.save -side right
}

proc fileSettings {{saveit 0}} {
  set descs "settings save file"
  if {$saveit} {
    if {[set f [openWriteFile gui "" name]] == 0} return
    puts $f "#$descs"
    puts $f "#version [globVal XcontrolVersion]"
    foreach g [savableSettings] {
      puts $f "gSet $g \{[globVal $g]\}"
    }
  } else {
    set name [fileDialog open gui]
    if {"" == [set f [openSaveFile $name $descs]]} return
    set errs ""
    while {[gets $f line] >= 0} {
      set sp [split $line]
      if {"gSet" != [lindex $sp 0]} continue
      set e [lindex $sp 1]
      if {! [isSavableSetting $e]} continue
      # match curly brace content
      if {[regexp "\{(.+)\}" $line a v]} {
        gSet $e "$v"
        #puts "gSet $e \"$v\""
      } elseif {[string match "*\{\}" $line]} {
        gSet $e ""
      }  else {
        set errs "!dubious input in $name ignored ($line)"
      }
    }
    if {$errs == ""} {set errs "control file $name successfully loaded"}
    applySettings
  }
  close $f
}

proc storeAll {extension {prosal ""} {as ""} {proto 1}} {
  if $proto conditionalOpenProtfile
  if {$extension == "gui"} {
    if {$as != ""} {
      if [catch {open $as w} f] {
	if $proto {outProtocol "can't open $as to write"}
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
    if $proto {
      set n [file tail $fname]
      set la [string last "." $n]
      if {$la > 0} {set n [string range $n 0 [incr la -1]]}
      setInstrumentfile $n
    }
  } else {
    if {[set f [openWriteFile $extension "" fname]] == 0} return
  }
  switch $extension {
    gui {
      cleanupGlobalVariables
      puts $f "#experiment description save file"
      puts $f "#version [globVal XcontrolVersion]"
      foreach g [savableGlobals] {
	puts $f "gSet $g \{[globVal $g]\}"
      }
    }
    bat {
      set c [generateVitessCommand bat]
      puts $f $c
    }
    default {
      puts $f [generateVitessCommand $extension]
    }
  }
  close $f
  if $proto {
    outProtocol "stored file $fname"
    conditionalCloseProtfile
  }
  saveLastState
}


proc doInsertPacked {name insert_after} {
  conditionalOpenProtfile
  set version -1
  if [catch {open $name r} f] { return 0 }
  # check if the file has been written by savePacket
  if {[gets $f] == "#packet file"} {
    if {1 != [scan [gets $f] "#version %d" version]} {
      set version 0
    }
  }
  if {$version == -1} {
    close $f
    outProtocol "! file $name missing or unappropriate"
    return 0
  }

  # read packet definitions from file
  set nkey {}
  set nval {}
  set nmods 0
  global DoNotSave DoNotSaveRegexp
  while {[gets $f line] >= 0} {
    set sp [split $line]
    if {"gSet" != [lindex $sp 0]} continue
    set k [lindex $sp 1]
    if [regexp $DoNotSaveRegexp $k] continue
    if {[lsearch $DoNotSave $k] >= 0} continue
    set v [lindex $sp 2]
    # remove braces from value
    if [catch {eval set v $v}] { set v ""}
    if [regexp {^mod([0-9]+)$} $k a n] {
      lappend nkey "mod[expr $n + $insert_after]"
      incr nmods
    } elseif [regexp {^(.+)_([0-9]+)$} $k a m n] {
      lappend nkey "${m}_[expr $n + $insert_after]"
    } else continue
    lappend nval "$v"
  }
  close $f
  if {$nmods == 0} {
    outProtocol "! file $name contains no module definitions"
    return 0
  }

  # save definitions of old modules known so far; shift corresponding keys
  set kkey {}
  set kval {}
  foreach g [savableGlobals] {
    set v [globVal $g]
    lappend kval "$v"
    if [regexp {^mod([0-9]+)$} $g a n] {
      if {$n > $insert_after} {incr n $nmods}
      lappend kkey mod$n
    } elseif [regexp {^(.+)_([0-9]+)$} $g a v n] {
      if {$n > $insert_after} {incr n $nmods}
      lappend kkey ${v}_$n
    } else {
      lappend kkey $g
    }
  }

  # delete old modules
  global defdirectory_ Mlf
  deleteSomeModules $Mlf 1

  setAll 0
  # set new global definitions
  foreach k $nkey v $nval {
    gSet $k $v
    #puts "gSet $k :$v:"
  }
  foreach k $kkey v $kval {
    gSet $k $v
    #puts "old gSet $k :$v:"
  }

  # create modules
  reShowModules $Mlf

  conditionalCloseProtfile
  return 1
}

proc insertPacket {w} {
  set insert_after [entryVal insmod]
  set name [entryVal ipacketfile]
  if {$name == ""} return
  if [doInsertPacked $name $insert_after] {
    catch {destroy $w}
  }
}

proc addPacket {} {
  set actm [activeModules]
  if {[llength $actm] <= 0} {set iat 0} else {set iat [lindex $actm end]}
  set name [fileDialog open gui]
  if {$name == ""} return
  doInsertPacked $name $iat
}

proc insertPacketWindow {} {
  set actm [activeModules]
  if {[llength $actm] <= 0} {
    addPacket
    return
  }
  set w .ipacket
  catch {destroy $w}
  generateToplevel $w "Insert packet"
  fGroup $w.v $w.b

  gSet insPacketESET [list [list insmod radio 1 {insert\nbehind module} $actm $actm] {
    ipacketfile browsefile "" {"package\nfilename" "Package modules will be inserted in the pipe."} r gui 1} ]
  generateEntries $w.v insPacketESET

  bButton $w.b.cancel Cancel "destroy $w"
  bButton $w.b.save "Insert Packet" "insertPacket $w"
  pack $w.b.cancel -side left
  pack $w.b.save -side right
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
    set v [set mv $DummyEntry]
    # dump old module name tag
    upvar #0 mmm_$i mt
    catch {unset mt}
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
  catch {destroy $w}
  generateToplevel $w "Set Instrument Name" "" +300+300
  fGroup $w.v $w.b
  generateEntries $w.v sInameESET

  bButton $w.b.cancel Cancel "destroy $w"
  bButton $w.b.save Set "doSetIname $w"
  pack $w.b.cancel -side left
  pack $w.b.save -side right
}

proc setInstrumentfile {name} {
  global instrumentfile
  regsub -all " " $name _ name
  set instrumentfile $name
  regexp {[0-9a-zA-ZäöüÄÖÜß_-]+} [file tail $instrumentfile] a

  .x.bm.hlab configure -text "Instrument $a" -font [bigLabelFont -3]
}

proc optVal {com c} {
  if {[regexp " -${c}(\[^ \]+)" $com a o]} {
    return $o
  }
  return ""
}

proc findModName {n com} {
  # return the module name for a given executable name n
  switch $n {
    chopper_fermi {
      switch [optVal $com O] {
        1 {return chopper_fermi_str}
        2 {return chopper_fermi_cur}
        default {return $n}
      }
    }
    lenses {return lense}
    monochr_analyser {
      switch [optVal $com O] {
        1 {return ma_flat}
        2 {return ma_focus}
        3 {return ma_focus_dat}
        default {return $n}
      }
    }
    monitor1 {
      switch [optVal $com k] {
        1 {return mon1_lambda}
        2 {return mon1_time}
        3 {return mon1_divy}
        4 {return mon1_divz}
        5 {return mon1_y}
        6 {return mon1_z}
        7 {return mon1_energy}
        8 {return mon1_divyz}
        default {return $n}
      }
    }
    mon2_posdiv {
      switch [optVal $com q] {
        1 {return mon2_y_divy}
        2 {return mon2_z_divz}
        default {return $n}
      }
    }
    monitorpol_1d {
      switch [optVal $com k] {
        1 {return monpol_lambda}
        2 {return monpol_time}
        3 {return monpol_divy}
        4 {return monpol_divz}
        5 {return monpol_y}
        6 {return monpol_z}
        default {return $n}
      }
    }
    sesans_field {return quadr_field}
    source {
      regexp {([^\/]+)\.mod} [optVal $com a] a oa
      set oa [string tolower $oa]
      switch [optVal $com S] {
        1 {
          switch -regexp $oa {
            hmi {return source_HMI}
            ill {return source_ILL}
            default {return source_const_wave}
          }
        }
        2 {
          switch -regexp $oa {
            ipns {return source_IPNS}
            parc {return source_J-PARC}
            sns {return source_SNS}
            isis {return source_ISIS}
            ess {return source_ESS}
            default {return source_short_pulsed}
          }
        }
        3 {return source_ESS_LPTS}
        default {return $n}
      }
    }
    default {
      upvar #0 ${n}ESET mm
      if [info exists mm] { return $n}
      return external_command
    }
  }
}

proc interpretCommand {com mi gsetkey gsetval} {
  set modname ""
  upvar $gsetkey gkey
  upvar $gsetval gval

  foreach c [split $com] {
    set c [string trim $c]
    if {$c == ""} continue
    if {$modname != ""} {
      if {"-" != [string range $c 0 0]} continue
      set k [string range $c 1 1]
      if {"-" == $k} {
        # a global options starting with --
        set v [string range $c 3 end]
        switch [string range $c 2 2] {
          G { lappend gkey gravity_
            if $v {set v on} else {set v off}
            lappend gval $v
          }
          U {lappend gkey wei_min_
            lappend gval $v
          }
          default {}
        }
        continue
      }
      set v [string range $c 2 end]
      # if the value v contains a dollar, this is probably from pipe variables
      # which are not accessible here
      if [regexp {\$} $v] {
        # extract a filename at the end if possible
        if [regexp {\/([^\/]+)$} $v a o] {
          set v $o
        }
      }
      set ais($k) $v
      #puts "  $k : $v"
    } else {
      # see if there is a / within
      if [regexp {\/([^\/]+)$} $c a fn] {
        # if it contains a .exe
        if {[regexp {^(.+)\.exe$} $fn a p]} {
          # got some Windows exe file
        } elseif {[regexp {^(.+)_(Linux|Darwin)} $fn a p op]} {
          # got some Linux or Mac image
        } else continue
        # cut off _parallel if seen
        if {[regexp {^(.+)_parallel$} $p a op]} {
          set p $op
        }
        set modname [findModName $p $com]
        if {$modname == ""} return ; # no chance to find a valid module
        lappend gkey mod$mi
        lappend gval $modname
        #puts "mod$mi -> $modname"
      }
    }
  }
  if {$modname == ""} { return 0}
  upvar \#0 ${modname}ESET modl
  foreach line $modl {
    set olist [lindex $line 3]
    set ochar [lindex $olist 3]
    if [info exists ais($ochar)] {
      set oname [lindex $line 0]
      set v $ais($ochar)
      if {"radio" == [lindex $line 1]} {
        # re-map radio itmes
        set i 0
        foreach vv [lindex $line 5] {
          if {$v == "$vv"} {
            set v [lindex [lindex $line 4] $i]
            break
          } else {
            incr i
          }
        }
      }
      lappend gkey ${oname}_$mi
      lappend gval $v
    }
  }
  return 1
}

proc doImportPipe {name} {
  # import an instrument from a pipe command in a file

  if [catch {open $name r} f] return

  set gsetkey {}
  set gsetval {}
  set mi 1
  while {[gets $f line] >= 0} {
    if [regexp {\|} $line] {
      foreach c [split $line |] {
        set c [string trim $c]
        if {$c == ""} continue
        if [interpretCommand $c $mi gsetkey gsetval] {
          incr mi
        }
      }
      break
    }
  }
  close $f
  if {$mi < 2} {
    outProtocol "--- could not parse pipe file $name to a VITESS instrument ---"
    return
  }

  # close all gui modules

  # remember old default directory
  global defdirectory_ Mlf
  set olddef $defdirectory_

  # this will probably be the new default directory
  set nd [file dirname $name]

  # delete all modules
  deleteSomeModules $Mlf 1

  # set global variables
  foreach k $gsetkey v $gsetval {
    gSet $k "$v"
  }

  setAll 0
  conditionalOpenProtfile
  outProtocol "--- pipe file $name successfully parsed ---"
  conditionalCloseProtfile

  # open gui modules

  reShowModules $Mlf

  removeTrailingDummies
  setInstrumentfile $name

  # Ask if modified new default directory is ok
  confirmedCommand gSet "defdirectory_ $nd" "Set default directory to $nd"
  saveLastState

  return 1
}

proc importPipe {} {
  set name [fileDialog open]
  if {$name == ""} return
  if [dontDoit "Your changes will be saved to a snapshot only. Continue importing a pipe?"] return
  doSnapshot
  doImportPipe $name
}


proc openSaveFile {name descs {versvar ""}} {
  # check if file has been written by a previous storeAll

  if {$name == ""} {return ""}

  if {$versvar != ""} {
    upvar $versvar version
  }
  set version -1
  if [catch {open $name r} f] {return ""}
  if {[gets $f] == "#$descs"} {
    if {1 != [scan [gets $f] "#version %d" version]} {
      set version 0
    }
  }
  if {$version == -1} {
    close $f
    outProtocol "! file $name is no instrument file"
    return ""
  }
  set a [globVal XcontrolVersion]
  if {int($version) != int($a)} {
    # versions with different integer part are incompatible
    close $f
    outProtocol "! version $version of $name doesn't match actual version $a"
    return ""
  }
  return $f
}

proc cleanupGlobalVariables {} {

  # Delete global <name>_<number> variables, if they do not belong to a valid module.
  # First we look for entry variables of active modules.
  global DummyEntry maxModule ENames

  for {set i 1} {$i <= $maxModule} {incr i} {
    upvar #0 mod$i mod
    if {! [info exists mod]} continue
    if {$mod == "$DummyEntry"} continue
    # check if it is a valid module name
    upvar #0 ${mod}ESET m
    if {! [info exists m]} continue
    set mname($i) $mod
    if [info exists ENames($mod)] continue ; # variable names of that module are known
    foreach n $m {
      set he [lindex $n 1]
      if {$he == "" || $he == "header"} continue
      lappend ENames($mod) [lindex $n 0]
    }
    lappend ENames($mod) mmm ; # special module name entry 
  }

  foreach e [info globals] {
    # we examine <name>_<number> variables.
    if {! [regexp {^(.+)_([0-9]+)$} $e a nm n]} continue
    if [info exists mname($n)] {
      # module n exists, look if the variable belongs to that module
      set rc [lsearch $ENames($mname($n)) $nm]
      # puts "   $mname($n) exists, $nm has index $rc"
      if {$rc >= 0} continue
      if  {[regexp {^(.+)_[or]_([0-9]+)$} $e a nm n]} {
        # do not delete name_o_n or name_r_n variables for known names
        set rc [lsearch $ENames($mname($n)) $nm]
        # puts "   $mname($n) exists, $nm has index $rc"
        if {$rc >= 0} continue
      }
    }
    # else delete that relict
    #dmf:debug
    #puts "unset $e"
    global $e
    unset $e
  }
}

proc assureConsistency {} {
  # As gui input files may contain inconsistent settings for various reasons.
  # We look for global variables which might disturb further work.
  # First we look for variables mod_<number>
  # These should be set to --inactive-- or a valid modul name.
  # All global variables of the form <name>_<number> are deleted, if they
  # do no belong to a active module.

  global maxModule DummyEntry

  for {set i 1} {$i <= $maxModule} {incr i} {
    upvar #0 mod$i mod
    set validmod($i) 0
    if [info exists mod] {
      if {$mod != "$DummyEntry"} {
        # check if it is a valid module name
        upvar #0 ${mod}ESET m
        if [info exists m] {
          set validmod($i) 1
          set activemod($mod) 1
        }
      }
    }
    if $validmod($i) continue
    set mod $DummyEntry
  }

  # Delete global <name>_<number> variables, if they do no belong to a valid module.
  foreach e [info globals] {
    if {[regexp {^mod([0-9]+)$} $e a n]} {
      if {$n >= 0 && $n <= $maxModule} continue
    } else {
      if {! [regexp {_([0-9]+)$} $e a n]} continue
      if {$n >= 0 && $n <= $maxModule && $validmod($n)} continue
    }
    global $e
    unset $e
  }

  # remember active (known, in this version defined, used here) modules
  foreach m [array names activemod] {
    upvar #0 ${m}ESET.active gact
    set gact 1
  }
}

###
proc loadAll {extension {givenname ""}} {

  set name $givenname
  if {$name == ""} {
    set name [fileDialog open $extension]
    if {$name == ""} return
  }

  if [dontDoit "Your changes will be saved to a snapshot only. Continue loading?"] return
  doSnapshot

  set f [openSaveFile $name "experiment description save file" version]
  if {$f == ""} return

  global defdirectory_ Mlf

  if {$givenname == ""} {
    # remember old default directory
    set olddef $defdirectory_

    # this will probably be the new default directory
    set nd [file dirname $name]
  }

  # delete all modules
  deleteSomeModules $Mlf 1

  # If a gui-file becomes loaded, settings for GUI sizes and the default directory
  # could be changed, too. If values come from a different OS, these values would be
  # non-sense or not applicable, at least these changes probably are unexpected.
  # So we check if a variable to load is allowed.

  global DoNotSave TempVars DoNotSaveRegexp

  set errs ""
  while {[gets $f line] >= 0} {
    set sp [split $line]
    if {"gSet" != [lindex $sp 0]} continue
    set e [lindex $sp 1]
    if [regexp $DoNotSaveRegexp $e] continue
    if {[lsearch $TempVars $e] >= 0} continue
    if {[lsearch $DoNotSave $e] >= 0} continue
    # match curly brace content
    if {[regexp "\{(.+)\}" $line a v]} {
      if {$version <= 2 && $v == "0"} {
        # Hack: some entry values have to be re-mapped 0 -> -1
        if [regexp {^(mtrl_colour|eval_colour|sn_eval_colour|si_color|detectcolor|treatcolor)_[0-9]+$} $e] {
          set v -1
        }
      }
      gSet $e "$v"
      #puts "gSet $e \"$v\""
    } elseif {[string match "*\{\}" $line]} {
      gSet $e ""
    }  else {
      set errs "!dubious input in $name ignored ($line)"
    }
  }
  close $f

  assureConsistency

  if {$errs == ""} {
    set errs "control file $name successfully loaded"
  }


  setAll 0
  conditionalOpenProtfile
  outProtocol "--- $errs ---"
  conditionalCloseProtfile

  reShowModules $Mlf

  removeTrailingDummies

  if {$givenname == ""} {
    setInstrumentfile $name

    # Ask if modified new default directory is ok
    confirmedCommand gSet "defdirectory_ $nd" "Set default directory to $nd"

  } else {
    global instrumentfile
    set oname $instrumentfile
    setInstrumentfile $oname
    set defdirectory_ [file dirname $oname]
  }

  cleanupGlobalVariables
  showModName
  saveLastState

  return 1
}

proc deleteAllModules {} {
  global Mlf Amf DoNotSaveRegexp
  deleteSomeModules $Mlf 1
  reShowModules $Mlf
  removeTrailingDummies
  setInstrumentfile 1
  gSet LastState ""
  helpFrame $Amf
  foreach e [info globals] {
    if [regexp $DoNotSaveRegexp $e] continue
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
	parfilename - pareditablefile - parbrowsefile -\
	    moneditablefile - mon2editablefile - mneditablefile - mn2editablefile {
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

proc saveTextFile {w fn kind {destroyAtEnd 1}} {
  if [catch {open $fn w} f] {
    showText "!!could not write $kind $fn"
  } else {
    puts $f [$w.v.text get 1.0 end]
    close $f
    showText "$kind $fn written"
  }
  if {$destroyAtEnd} {
    destroy $w
  }
}

proc showTextEditWindow {w fn kind height {dowarn 0} {width 150}} {
  global monospaced bgColor
  catch {destroy $w}
  generateToplevel $w "Edit $kind"
  fGroup $w.v $w.b
  if {$width > 100} {set fontsize 8} else {set fontsize 9}
  text $w.v.text -relief raised -bd 2 \
      -height $height -width $width\
      -font [list $monospaced 8 normal] -bg $bgColor\
      -setgrid 1\
      -yscrollcommand "$w.v.yscroll set"
  yscroll $w.v "$w.v.text yview"
  pack $w.v.text -side left -fill both -expand yes
  if [catch {open $fn r} f] {
    if {$dowarn} {showText "old file $fn did not exist"}
  } else {
    while {[gets $f line] >= 0} {$w.v.text insert end "$line\n"}
    close $f
  }
  bButton $w.b.save Save [list saveTextFile $w $fn "$kind" 0]
  bButton $w.b.savecl Save+Close [list saveTextFile $w $fn "$kind"]
  bButton $w.b.delcan Delete+Close "file delete $fn; destroy $w"
  bButton $w.b.cancel Cancel "destroy $w"
  pack $w.b.save $w.b.savecl $w.b.delcan $w.b.cancel -side left -expand 1
}

proc editInfFile {{mode 0}} {
  if $mode {set ft open} else {set ft write}
  if {[set fn [fileDialog $ft inf instrument.inf]] == 0} return
  showTextEditWindow .editinf $fn "Instrument File" 32 1
}

proc lCompare  {a b} {
  global FTime
  set xa [lindex $FTime $a]
  set xb [lindex $FTime $b]
  if {$xa > $xb} {return -1}
  if {$xa == $xb} {return 0}
  return 1
}

proc recoverFile {w fn} {
  # who recoveres a file will probably need more snapshots
  global StoreStates
  if {$StoreStates < 8} {set StoreStates 8}

  # dump the recovery GUI window
  destroy $w

  # load the old snapshot
  loadAll gui $fn
}

proc recoverFileGUI {} {
  set w .rgui
  if [winfo exists $w] {
    raise $w
    return
  }
  global FTime bgColor
  set fdir [file join [globVal SourceDirectory] FILES .saved]
  set i 0
  set FTime {}
  set fname {}
  foreach fn [glob -nocomplain -directory $fdir *.gui] {
    lappend FTime [file mtime $fn]
    lappend fname $fn
    lappend findex $i
    incr i
  }
  if {$i == 0} {
    showText "!No snapshots taken so far"
    return
  }
  set findex [lsort -command lCompare $findex]

  set n "Recover Instrument"
  generateToplevel $w $n "" "+120+60"
  label $w.l -text $n -font [headerFont] -bg $bgColor
  pack $w.l -side top -fill both -expand yes

  set now [clock seconds]
  for {set k 0} {$k < $i} {incr k} {
    set ui [lindex $findex $k]
    set tfn [lindex $fname $ui]
    set ot [lindex $FTime $ui]
    set ago [expr $now - $ot]
    if {$ago > 3600} {
      set ttime [clock format $ot -format {%H:%M:%S   %d.%m.%Y}]
    } elseif {$ago > 60} {
      set minutes  [expr int($ago / 60)]
      set sec [expr $ago - 60*$minutes]
      set ms [format  {%2d} $minutes]
      set ss [format  {%0.2d} $sec]
      set ttime  "$ms:$ss minutes ago"
    } else {
      set ttime "$ago seconds ago"
    }
    bButton $w.b$k $ttime "recoverFile $w $tfn"
    pack $w.b$k -side top -fill both -expand yes
  }

  bButton $w.b$k Cancel "destroy $w"
  pack $w.b$k -side top -fill both -expand yes
}
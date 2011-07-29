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
    if {$v != "--inactive--" } {
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
    if {[set f [openWriteFile $extension "" fname]] == 0} return
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
  outProtocol "stored file $fname"
  conditionalCloseProtfile
  gSet LastState [generateVitessCommand kstate]
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

proc openSaveFile {name descs} {
  # check if file has been written by a previous storeAll

  if {$name == ""} return ""

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
    close $f
    outProtocol "! version $version of $name doesn't match actual version $a"
    return ""
  }
  return $f
}

###
proc loadAll {extension} {
  if [dontDoit "You have unsaved changes. Forget them?"] return

  set name [fileDialog open $extension]
  set f [openSaveFile $name "experiment description save file"]
  if {$f == ""} return
 
  # remember old default directory
  global defdirectory_ Mlf
  set olddef $defdirectory_

  # this will probably be the new default directory
  set nd [file dirname $name]

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
      gSet $e "$v"
      #puts "gSet $e \"$v\""
    } elseif {[string match "*\{\}" $line]} {
      gSet $e ""
    }  else {
      set errs "!dubious input in $name ignored ($line)"
    }
  }
  close $f
  if {$errs == ""} {
    set errs "control file $name successfully loaded"
  }

  setAll 0
  conditionalOpenProtfile
  outProtocol "--- $errs ---"
  conditionalCloseProtfile

  reShowModules $Mlf

  removeTrailingDummies
  setInstrumentfile $name

  # Ask if modified new default directory is ok
  confirmedCommand gSet "defdirectory_ $nd" "Set default directory to $nd"
  gSet LastState [generateVitessCommand kstate]

  return 1
}

proc deleteAllModules {} {
  # delete all modules
  global Mlf Amf DoNotSaveRegexp
  deleteSomeModules $Mlf 1
  reShowModules $Mlf
  removeTrailingDummies
  setInstrumentfile 1
  gSet LastState ""
  helpFrame $Amf
  foreach e [stringToSet [info globals]] {
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

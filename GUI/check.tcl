### project Xcontrol
### HMI DN
### M. Fromme fromme@hmi.de

### Force entry filename to be plain file name.
### If a full directory path is given, ask to copy the
### file to the default (parameter) directory.
### returncode 1 means an invalid directory has been specified.
### filename and fullname arguments is passed by reference!
###
proc forceParamDir {{filename ""} {fullname ""}} {

  upvar #0 defdirectory_ pdir
  if {$pdir == ""} {
    set pdir [pwd]
    showText "default parameter file directory has been set to $pdir"
  }
  if {![file exists $pdir]} {
    confirmedCommand mkdir $pdir "create directory $pdir"
    set padir $pdir
  }
  if {![file isdirectory $pdir]} {
    showText "!$pdir is no valid directory"
    return 1
  }
  if {![file writable $pdir]} {
    showText "!You are not allowed to write to default directory $pdir"
    return 1
  }
  if {$filename == ""} {return 0}

  upvar $filename v
  set fn [file tail $v]
  set dir [getDirectory $v]
  if {$fullname != ""} {upvar $fullname resfn}
  set resfn [file join $pdir $fn]
  if {$dir != "." && $dir != $pdir} {
    if [file exists $v] {
      confirmedCommand cp "$v $pdir"\
	  "copy parameter file\n$fn\nto\n$pdir"
    }
    showText "!parameter file $fn must reside in $pdir"
    if {![file exists $resfn]} {return 1}
  }
  set v $fn;		# leave plain file name
  return 0
}

###
### tools to check variables
###

### check if value v, entry name, confirms to given format
###
proc scanForm {v format name} {
  if {$v == ""} {
    showText "! no value for $name"
    return ""
  }
  set dummy ""
  set c [scan $v "$format %s" vv dummy]
  if {"" != [string trim $dummy]} {
    set c "!$name: found extra characters in \"$v\""
    if {$format == "%i"} {
      append c "  (must be integer)"
    }
    showText $c
    return ""
  }
  if {1 == $c} {
    return $vv
  }
  # try expression
  if {![catch {expr $v} i]} {
    return $i
  }
  if {$format == "%i"} {
    showText "! $name must be integer, found \"$v\""
  } else {
    showText "! $name must be float, found \"$v\""
  }
  return ""
}

### check if a value v, entry name, is out of range [l,u]
###
proc valOutofRange {v l u name} {
  if {$v == "" || $v < $l || $v > $u} {
    showText "! $v off limits: $l <= $name <= $u"
    return 1
  }
  return 0
}
proc failedCompare {name v e op} {
  if {$e == ""} {return 0}
  if [string match *\[a-z\]* $e] {
    set op [string range $e 0 1]
    set e  [string range $e 2 end]
  }
  switch $op {
    le {set r [expr $v <= $e]}
    lt {set r [expr $v < $e]}
    eq {set r [expr $v == $e]}
    ge {set r [expr $v >= $e]}
    gt {set r [expr $v > $e]}
    default {set r 0}
  }
  if $r {return 0}
  showText "! value for $name off limits : $v is not $op $e"
  return 1
}
proc outofRange {name var format l u mandatory} {
  upvar $var v
  set vv [scanForm $v $format $name]
  if {$vv == ""} {return 1}
  if {$v != $vv} {set v $vv }
  if {$mandatory && $v == ""} {
    showText "!Please specify a value for $name"
    return 1
  }
  if [failedCompare $name $v $l ge] {return 1}
  return [failedCompare $name $v $u le]
}

### check if value of global variable is conformant with description
###   in variable description line/list
###   check by definition in definition line e
###   (see description in e.g. vitess.tcl)
###
proc errorInLine {e app {mod ""}} {
  set var [lindex $e 0]
  if {$var == ""} {return 0};		# just separator item
  set type [lindex $e 1]
  set list [lindex $e 3]
  if {$list == ""} {return 0};		# just header item
  set arg4 [lindex $e 4]
  set arg5 [lindex $e 5]
  set arg6 [lindex $e 6]
  set mandatory 0
  if {$arg6 == "1"} {
    set mandatory 1
  } elseif {$arg4 == "1" && $arg5 == ""} {
    set mandatory 1
    set arg4 ""
  }

  set name [lindex $list 0]
  regsub -all {[^].a-zA-Z0-9()[_-]} $name " " name
  upvar #0 $var$app v
  if [info exists v] {set v [string trim $v]} else {set v ""}
  if {$v == ""} {			# blank input
    if $mandatory {			# on necessary item
      showText "!Please specify $name as $type"
      return 1
    }
    if {$type != "radio"} {
      return 0;				# on unchecked item
    }
  }

  switch $type {
    int   {return [outofRange $name v "%i" $arg4 $arg5 $mandatory]}
    float {return [outofRange $name v "%f" $arg4 $arg5 $mandatory]}
    radio {
      # empty strings come from elder *.gui files here, just accept
      if {$v == ""} {return 0}
      foreach it $arg4 {
	if {$v == $it} {return 0}
      }
      showText "!Please select an option for $name"
      return 1
    }
    filename - editablefile - browsefile - browsedir - \
	parfilename - pareditablefile - parbrowsefile - moneditablefile - mon2editablefile {
	  switch $type {
	    filename - editablefile - browsefile - browsedir {
	      set dir [getDirectory $v]
	      if  {![file exists $dir]} {
		showText "!Please create directory $dir before using it for $name."
		return 1
	      }
	      set resfn $v;		# really used filename
	    }
	    default {
	      forceParamDir v resfn
	    }
	  }
	  if {$arg4 == "r"} {
	    if [catch {glob $resfn} resname] {
	      showText "! $mod file not found, respecify $name"
	      return 1
	    }
	  }
	}
  }
  return 0
}

### write entry value with command option
### to global command string
proc writeCommandOption {e {app _} {special ""} {serpar {}} {serrep {}} {serno {}}} {
  set list [lindex $e 3]
  if {[set c [lindex $list 3]] == ""} return ; # command option
  set varname [lindex $e 0]
  set v [string trim [entryVal $varname $app]];	# value
  if {$v == ""}  {
    if {$special == ""} return
    set v $special
  }
  global FullCommand Comode Serdefault SerRadio SerRadioV Plotfile Plottype

  # find if a parameter is to be replaced for series simulation
  set vser ""
  set sercol 0
  foreach p $serpar {
    if {$c == $p} {
      set vser [lindex $serrep $sercol]
      break
    }
    incr sercol
  }
  switch [set rt [lindex $e 1]] {
    radio {
      # substitute visible values with command codes, if
      # command codes are present
      if {"" != [set vlist [lindex $e 5]]} {
	set rlist [lindex $e 4]
	if {$vser != ""} {
	  set SerRadio($sercol) [join $rlist ":"]
	  set SerRadioV($sercol) [join $vlist ":"]
	}	
	foreach it $rlist vrep $vlist {
	  if {$v == $it} {
	    set v $vrep
	  }
	  if {$vser == $it} {
	    set vser $vrep
	  }
	}
      }
    }
    parfilename - pareditablefile - parbrowsefile - moneditablefile - mon2editablefile {
      if {$special == "" || $v != $special} {
	set ptail [file tail $v]
	switch $Comode {
	  bat - tcl - ser {
	    if {$vser != ""} {
	      set v $ptail
	      set vser [file join \$P $vser]
	    } else {
	      set v [file join \$P $ptail]
	    }
	  }
	  default   {
	    set v [file join [getDirectory [entryVal defdirectory]] $ptail]
	    switch $rt {
	      moneditablefile {set tt 1}
	      mon2editablefile {set tt 2}
	      default {set tt 0}
	    }
	    if {$tt > 0} {
	      set vv [entryVal ${varname}_r $app]
	      if {$vv == "" || $vv == "1"} {
		lappend Plotfile $v
		lappend Plottype $tt
	      }
	    }
	  }
	}
      }
    }
  }
  if {$vser == ""} {
    append FullCommand " -$c$v"
    return
  }
  # remember GUI setting for Serdefault
  foreach p $serpar sno $serno {
    if {$c == $p} {
      set Serdefault($sno) $v
      break
    }
  }
  append FullCommand " -$c$vser"
}


### decide if a module is active
###
proc activeModule {mod} {
  upvar #0 ${mod}ESET.active gact
  if [info exists gact] {return $mod}
  return ""
}

### check all variables given by global list list
###
proc errorWithValues {mod {showok 1} {app _}} {
  if {[activeModule $mod] == ""} {return 0}
  showText "\tchecking [string toupper $mod]"
  set errors 0
  foreach l [globVal ${mod}ESET] {
    if [errorInLine $l $app $mod] {set errors 1}
  }
  if {[set tp [info proc ${mod}CheckErr]] != ""} {
    if [catch {eval $tp $app} res] {
      puts "Error within proc $tp"
    } elseif {$res != "0"} {
      return 1
    }
  }
  if {$errors} {return 1}
  if {$showok} {showText "$mod O.K."}
  return 0
}


proc checkMiMaErr {a b name {app _}} {
  if {[set mi [entryVal $a $app]] == ""} {return 0}
  if {[set ma [entryVal $b $app]] == ""} {return 0}
  if {$mi > $ma} {
      showText "! minimal $name should be less or equal maximal $name"
      return 1
    }
  if {$name == ""} {return 0}
  if {[set v [entryVal $name $app]] == ""} {return 0}
  if {$mi > $v || $v > $ma} {
    showText "! $mi <= $name <= $ma"
    return 1
  }
  return 0
}


### find variable description line/list for a given variable
###   search all defintions of set (default neededModulesSET)
###   return "" if not found
###
proc findControlvarLine {name} {
  global neededModulesSET
  foreach line $neededModulesSET {
    foreach set [lindex $line 2] {
      foreach l [globVal $set] {
	if {[lindex $l 0] == "$name"} {
	  return $l
	}
      }
    }
  }
  return ""
}

### find module for a given control variable
###
proc findModuleLine {name} {
  global neededModulesSET
  set n [string tolower $name]
  foreach g $neededModulesSET {
    foreach set [lindex $g 2] {
      foreach l [globVal $set] {
	if {[lindex $l 0] == "$n"} {
	  return $g
	}
      }
    }
  }
  return ""
}

###
### find variable line in a given set
###
proc findLineInSet {name globset} {
  upvar #0 $globset set
  foreach g $set {
    if {[lindex $g 0] == "$name"} {
      return $g
    }
  }
  return ""
}

### find variable description line/list for a given variable
###   search all defintions of globalDescriptionSET
###
proc findDescriptionLine {name} {
  global globalDescriptionSET
  set n [string tolower $name]
  foreach g $globalDescriptionSET {
    upvar #0 ${n}.active gact
    if {[info exists gact] && [winfo exists $gact]} {
      foreach l [globVal $g] {
	if {[lindex $l 0] == "$n"} {
	  return $l
	}
      }
    }
  }
  return ""
}

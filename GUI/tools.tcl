### project Xcontrol
### HMI DN
### M. Fromme fromme@hmi.de
### June 1999

###
### general tools
###

proc itemize {s {schar ""}} {
  # split a string Perl-like
  if {$schar == ""} {
    set sp [split $s]
  } else {
    set sp [split $s $schar]
  }
  set res {}
  foreach e $sp {
    if {$e == ""} continue
    lappend res $e
  }
  return $res
}

proc padString {v n} {
  if {[string length $v] > 40} {
    return [string range $v 0 39]
  }
  return [format "%${n}s" $v]
}

proc padS {v {n 12}} {
  set fm "%${n}s"
  if {[set zlen [string first \n $v]] >= 0} {
    set fl [format $fm [string range $v 0 $zlen]]
    set sl [format $fm [string range $v [expr $zlen + 1] end]]
    return "$fl$sl"
  }
  if {[string length $v] > 40} {
    return [string range $v 0 39]
  }
  return [format $fm $v]
}

proc removeSubwindows {w} {
  if {![winfo exists $w]} return
  foreach wd [lsort -decreasing [split [winfo children $w]]] {
    destroy $wd
  }
}

proc incrF {v {delta 1}} {
  upvar $v x
  set x [expr $x + $delta]
}

proc getNumAndUnit {s ni ui} {
  upvar $ni n
  upvar $ui u
  set s [string trim $s]
  if {![regexp {(.+)([cm])$} $s a n u]} {
    set n $s
    set u _
  }
  if [catch {expr 0 + $n} n] {
    set n 0
  }
}

proc addNumUnitVals {a b} {
  getNumAndUnit $a an au
  getNumAndUnit $b bn bu
  switch $au$bu {
    _c - c_ - cc {return "[expr $an + $bn]c"}
    _m - m_ - mm {return "[expr $an + $bn]m"}
    cm      {return "[expr 10 * $an + $bn]m"}
    mc      {return "[expr $an + 10 * $bn]m"}
    default {return [expr $an + $bn]}
  }
}

### easy framing
###
proc lFrame w {
  global bgColor
  frame $w -bg $bgColor
  pack $w -side left -pady 1m
}

proc tFrame w {
  global bgColor
  frame $w -bg $bgColor
  pack $w -side top -fill both -padx 10
}

proc Frame {w {packexp no}} {
  global bgColor
  frame $w -bg $bgColor
  pack $w -side top -fill both -expand $packexp
}

proc fGroup {args} {
  global bgColor
  foreach g $args {frame $g -bg $bgColor}
  eval "pack $args -side top -fill both -expand yes"
}
proc mGroup {dy args} {
  global bgColor
  foreach g $args {
    frame $g -bg $bgColor
    pack  $g -side top -fill both -pady $dy
  }
}

proc scrollFrame {w side cw ch sh {sw ""}} {
  global bgColor
  if {$side == "both"} {
    set mf $w.modf
    frame $mf
    pack $mf -side top -fill both -expand yes
    set c $mf.c
    yscroll $mf "$c yview"
    if {$sw == ""} {set sw [addNumUnitVals $cw 4c] }
    canvas $c -width $cw -height $ch -bg $bgColor\
	-scrollregion [list 0 0 $sw $sh]\
	-xscrollcommand "$w.xscroll set" -yscrollcommand "$mf.yscroll set"
    pack $c -side left -expand yes -fill both
    xscroll $w "$c xview"
  } else {
    set c $w.c
    yscroll $w "$c yview" $side
    canvas $c -width $cw -height $ch -bg $bgColor\
	-scrollregion [list 0 0 $cw $sh]\
	-yscrollcommand "$w.yscroll set"
    pack $c -side $side -fill both -expand yes
  }
  set wmf $c.f
  frame $wmf -bg $bgColor -width $cw -height $ch
  $c create window 0 0 -window $wmf -anchor nw
  return $wmf
}

proc lLabel {w {text ""}} {
  global labColor
  label $w.label -text $text -font [labelFont] -bg $labColor
  pack  $w.label -side top -fill both
}

proc sLabel {w {text ""}} {
  global labColor
  label $w.label -text $text -font [headerFont] -bg $labColor
  pack  $w.label -side top -fill both
}

proc helpLink {w s m {item ""}} {
  if {$item == ""} {set item $s}
  $w tag bind $m <ButtonPress> [list showHelpItem $item]
  $w insert end $s [list tl $m]
}

proc helpButton {w s m {item ""}} {
  global buttonColor
  if {$item == ""} {set item $s}
  button $w.$m -text $s -command [list showHelpItem $item]\
      -font [sbuttonFont] -background $buttonColor
  $w window create end -window $w.$m
}

proc helpFrame {w {mode ""}} {
  global bgColor buttonColor
  set w $w.helpframe
  frame $w
  pack $w -expand yes -fill both -padx 4
  set w $w.t
  text $w -relief raised \
      -height 100 -width 132\
      -font [textFont] -bg $bgColor\
      -wrap none
  pack $w -side left -expand yes -fill both -ipadx 1 -ipady 1
  #$w tag configure t1 -font [labelFont]
  $w tag configure tl -underline on -font [labelFont]

  helpButton $w {Getting Help} t5 Help
  $w insert end {

You can get help about every
  - parameter by clicking on its name (see also help for module)
  - module by clicking on the module number or choosing the menu Help

}
  helpLink $w Help t10
  $w insert end {

Alternativly, you can use the help system in the internet:
}

  helpLink $w http://www.hmi.de/projects/ess/vitess/DOC/index.html t11
  $w insert end {

For further questions, please send an email to vitess@hmi.de

}

  helpButton $w {Getting Started} t20
  helpButton $w Tutorial t24 tutorial.pdf
  $w insert end \n
  helpButton $w {Inserting/Deleting a Module} t21
  helpButton $w {Visualsing Results} t22
  $w insert end \n
  helpButton $w Troubleshooting t23
}


proc pLabel {w {text ""}} {
  global bgColor
  label $w.label -text $text -font [headerFont] -bg $bgColor
  pack  $w.label -side top -fill both -padx 0.5m -pady 0.5m
}

proc headLine {w text} {
  global bgColor
  label $w.headline -text $text -font [headerFont] -bg $bgColor
  pack  $w.headline -side top -fill both -padx 0.5m -pady 0.5m
}
proc bButton {w text command} {
  global buttonColor
  button $w  -font [buttonFont] -text $text -command $command\
      -background $buttonColor
}
proc bPack {args} {
  foreach but $args {
    pack $but -side left -ipadx 1m
  }
}

proc dismissFrame {w {d dism}} {
  global bgColor
  frame $w.$d -bg $bgColor
  pack $w.$d -side bottom -fill x -pady 2m
  bButton $w.$d.dismiss Dismiss "destroy $w"
  pack $w.$d.dismiss -side left
}

###
### tools to set and check global variables
###
proc checkalldefs {} {
  global globalDescriptionSET
  set elist {}
  foreach g [lsort [split [info globals]]] {
    if [string match *ESET $g] {
      lappend elist $g
    }
  }
  foreach mod $elist {
    puts "module : $mod"
    upvar #0 $mod m
    set nlist {}
    set olist {}
    foreach n $m {
      if {"" == [set name [lindex $n 0]]} continue
      if {"header" == [lindex $n 1]} continue
      if {[lsearch $nlist $name] == -1} {
	lappend nlist $name
      } else {
	puts "\tdouble name :$name:"
      }
      if {"" == [set opts [lindex $n 3]]} continue
      if {"" == [set opt [lindex $opts 3]]} continue
      if {[lsearch $olist $opt] == -1} {
	lappend olist $opt
      } else {
	puts "\tdouble option :$opt:"
      }
    }
    set lnl [llength $nlist]
    puts "  $lnl [lsort $nlist]"
    set onl [llength $olist]
    puts "  $onl [lsort $olist]"
  }
}

### get value of global variable
###                if variable has not been defined, return ""
proc globVal {v} {
  upvar #0 $v locv
  if [info exists locv] {
    return $locv
  }
  return ""
}


### return 0 if v is part of args list
###
proc isNot {v args} {
  foreach t $args {
    if [string match $t $v] {
      return 0
    }
  }
  return 1
}

### if global variable a is not known, then define a with value
###
proc forceDef {a val} {
  upvar #0 $a v
  if [info exists v] return
  set v $val
}
### force empty string definitions of global variables, if necessary
###
proc forceDefs {args} {
  foreach a $args {
    global $a
    if [info exists $a] continue
    set $a ""
  }
}
### short form to set global variable
###
proc gSet {a b} {
  global $a
  set $a $b
}

proc gsetBlank {args} {
  foreach a $args {
    global $a
    set $a {}
  }
}

###
### split a string given by a global variable name to a list.
### return unique items.
### "b a c b" becomes {a b c}

proc stringToSet {s} {
  set list {}; set e ""
  foreach l [lsort [split $s]] {
    if {$e != $l} {
      lappend list $l
    }
    set e $l
  }
  return $list
}

proc addToSet {set v} {
  upvar $set s
  if {[lsearch $s $v] == -1} {
    lappend s $v
  }
}

### return list of all globals which
### 1. are not defined internally by Tcl/Tk (may change)
### 2. do not start with an uppercase letter or .
### 3. do not end with SET or Add
### 4. are not an array variable
### 5. do not belong to inactive modules after the last active one
###
proc savableGlobals {} {
  global maxModule DummyEntry TempVars
  set lasti 0
  for {set i 1} {$i <= $maxModule} {incr i} {
    set varName mod$i
    upvar #0 $varName var
    if {![info exists var] || $var == $DummyEntry} continue
    set lasti $i
  }
  set l {}
  foreach e [stringToSet [info globals]] {
    if [regexp {^([A-Z_.]|error|auto_|arg|tk|tcl|blt_)|env|(SET|Add|Outstring)$} $e] continue
    if [regexp {_([0-9]+)$} $e a n] {
      if {$n > $lasti} continue
    }
    if {[lsearch $TempVars $e] >= 0} continue
    global $e
    if {[catch {array size $e} size] || !$size} {
      lappend l $e
    }
  }
  return $l
}


proc nextNumItems {f n result} {
  upvar $result ores
  set res {}
  set read 0
  while {[gets $f line] >= 0} {
    set line [string trim $line]
    if {$line == ""} continue
    switch [set i [scan $line "%g%g%g%g%g%g%g%g%g%g" n1 n2 n3 n4 n5 n6 n7 n8 n9 n10]] {
      10 {lappend res $n1 $n2 $n3 $n4 $n5 $n6 $n7 $n8 $n9 $n10}
      9 {lappend res $n1 $n2 $n3 $n4 $n5 $n6 $n7 $n8 $n9}
      8 {lappend res $n1 $n2 $n3 $n4 $n5 $n6 $n7 $n8}
      7 {lappend res $n1 $n2 $n3 $n4 $n5 $n6 $n7}
      6 {lappend res $n1 $n2 $n3 $n4 $n5 $n6}
      5 {lappend res $n1 $n2 $n3 $n4 $n5}
      4 {lappend res $n1 $n2 $n3 $n4}
      3 {lappend res $n1 $n2 $n3}
      2 {lappend res $n1 $n2}
      1 {lappend res $n1}
      default {set i 0}
    }
    if {[incr read $i] >= $n} break
  }
  set ores {}
  foreach i $res {
    set ti [expr int($i)]
    if {$ti == $i} {
      lappend ores $ti
    } else {
      lappend ores $i
    }
  }
  return $read
}

proc readNumItems {f alist app} {
  foreach l $alist {
    upvar #0 $l$app $l
  }
  set l [llength $alist]
  if {[nextNumItems $f $l ni] >= $l} {
    foreach it $alist n $ni {
      set $it $n
    }
    return 1
  }
  return 0
}

###
### re-poll a callback
###
proc rePoll {callback} {
  global polls_ Protfile
  if {$Protfile != ""} {flush $Protfile}
  if {1 != [scan $polls_ "%f" poll]} {
    set polls_ 1
    after 1000 $callback
    return
  }
  if {$polls_ < 0.5} {set polls_ 0.5}
  if {$polls_ > 10} {set polls_ 10}
  after [expr int(1000 * $polls_)] $callback
}

###
### protocol utilities
###
###

### open protocol file
### try to use default parameter directory

proc openProtfile {{mode a+}} {
  global Protfile ProtfileName defdirectory_

  set dayname "Xc[clock format [clock seconds] -format "%Y%j"].log"
  set fn [file join $defdirectory_ $dayname]
  if [info exists ProtfileName] {
    if {$ProtfileName == $fn && $Protfile != ""} return
    if {$Protfile != ""} {
      close $Protfile
    }
  }
  set Protfile ""
  if [catch {open $fn $mode} Protfile] {
    # current directory is not writeable, so use Unix home or C:
    if {[getSystem] == "unix"} {set fn "~/"} else {set fn "C:/"}
    append fn $dayname
    if [catch {open $fn $mode} Protfile] {
      showText "!unable to open protocol file"
      return
    }
  }
  set ProtfileName $fn
}


proc closeProtfile {} {
  global Protfile
  if {$Protfile == ""} return
  close $Protfile
  set Protfile ""
}
proc flushProtfile {} {
  global Protfile
  if {$Protfile != ""} {
    flush $Protfile
  }
}

proc outProtocol {text {simu 0}} {
  if {$simu} {
    set text "SIMUL $text"
  }
  global Protfile
  if {$Protfile == ""} {
    showText $text
    return
  }
  set text [showText $text]
  puts $Protfile "[clock format [clock seconds] -format "%d.%m.%Y %H:%M:%S"] $text"
}

proc conditionalOpenProtfile {} {
  global ProtocolMode
  if {$ProtocolMode != "nothing"} openProtfile
}

proc conditionalCloseProtfile {} {
  global ProtocolMode
  if {$ProtocolMode == "action"} closeProtfile
}

proc dontDoit text {
  global LastState
  set newState [generateVitessCommand action]
  if {$newState == "" || $LastState == $newState} {return 0}
  set rc [tk_messageBox -icon question -type yesno\
	      -title "confirmed command" -message $text]
  if {$rc == "no"} {return 1}
  return 0
}

proc confirmedCommand {command args {text ""}} {
  if {$text == ""} {
    set text "$command $args ?"
  }
  if [dontDoit $text] return
  switch $command {
    mkdir   {eval file mkdir $args}
    cp      {eval file copy $args}
    default {eval "$command $args"}
  }
}

proc selectCommand {
  yescommand
  yestext
  nocommand
  notext
  {text "Please Select"}
} {
  if [tk_dialog .confirmbox Alternative $text questhead 0 $yestext $notext] {
    eval $nocommand
  } else {
    eval $yescommand
  }
}

proc tabOut {f args} {
  set os ""
  foreach a $args {
    if {[string length $a] > 7} {
      append os "$a\t"
    } else {
      append os "$a\t\t"
    }
  }
  puts $f $os
}

###
### add selected items to a list
###
proc addToVars {w selentry sublist glist} {
  upvar #0 $selentry sel
  upvar #0 $sublist sellist
  upvar #0 $glist list
  foreach i [$w.l.list curselection] {
    addToSet sellist [lindex $list $i]
  }
  global tk_version
  if {$tk_version < 4.0} {
    $w.l.list select clear
  } else {
    $w.l.list selection clear 0 end
  }
  set sel [join $sellist]
}


proc startHeavyExecution {} {
  global HEAVY
  set HEAVY 1
}
proc checkHeavyExecution {} {
  update
  global HEAVY
  return $HEAVY
}
proc stopHeavyExecution {} {
  global HEAVY
  set HEAVY 0
}

proc randomInit {{seed 0}} {
  global rand
  set rand(ia) 9301
  set rand(ic) 49297
  set rand(im) 233280
  set rand(seed) $seed
}
proc randomV {} {
  global rand
  if {![info exists rand]} randomInit
  set rand(seed) [expr $rand(seed)*$rand(ia) + $rand(ic) % $rand(im)]
  return [expr $rand(seed)/double($rand(im))]
}
proc randomRange {{range 1000}} {
  global rand
  return [expr int([randomV]*$range)]
}

proc getSystem {} {
  global tcl_platform
  return $tcl_platform(platform)
}

proc tmpFilename {{name temp.tmp}} {
  if {[getSystem] != "windows"} {
    return "/tmp/[exec whoami]$name"
  }
  global defdirectory_
  set d $defdirectory_
  if {$d == "" || ! [file isdirectory $d]} {
    set d C:/temp
    # create C:/temp if not existing
    if [catch {file mkdir $d}] {
      return $name
    }
  }
  return [file join $d $name]
}

proc getDirectory {name} {
  if [file isdirectory $name] {return $name}
  return [file dirname $name]
}

proc parFileReadable {name app} {
  global defdirectory_
  return [file readable [file join $defdirectory_ [entryVal $name $app]]]
}

proc browseFile {var access dirtype {ext ""} {mustexist false}} {
  set err 1
  if {$dirtype == "d"} {
    set err [catch {tk_chooseDirectory -mustexist $mustexist} name]
  }
  if $err {if {[set name [fileDialog $access $ext]] == ""} return}
  global $var
  switch $dirtype {
    d {
      if {! [file isdirectory $name]} {
	# user has given a/b/c
	if [file exists $name] {
	  # a/b/c exists as plain file
	  # so we use a/b/
	  set name [file dirname $name]
	} else {
	  # uses tries to create directory a/b/c/
	  file mkdir $name
	}
      }
    }
    p {
      set fname $name
      set name [file tail $name]
      if [file readable $fname] {
	global defdirectory_
	set rname [file join $defdirectory_ $name]
	if {$fname != $rname} {
	  if [file exists $rname] {
	    confirmedCommand cp "-force $fname $rname" \
		"Overwrite\n$name\nwith parameter file from\n$$defdirectory_"
	  } else {
	    confirmedCommand cp "$fname $rname" \
		"Copy parameter file\n$name from\n[file dirname $fname]\nto default directory"
	  }
	}
      }
    }
  }
  set $var $name
}


proc saveFile {w fn {saveAs 0}} {
  if $saveAs {set fn [fileDialog write "" $fn]}

  if [catch {open $fn w} f] {
    showText "can't open $fn to write"
    return
  }
  puts $f [$w.v.text get 1.0 end]
  close $f
  destroy $w
}

proc openWriteFile {ext {sugg ""} {fname ""}} {
  if {$fname != ""} {upvar $fname n}
  set n [fileDialog write $ext $sugg]
  if {$n == ""} {return 0}
# code excluded, because fileDialog already asked if necessary
#  if {![catch {glob $n}]} {
#    if {[tk_dialog .confirmbox "confirm" \
#	     "replace file $n ?" questhead 0 replace cancel]} {
#      return 0
#    }
#  }
  if [catch {open $n w} f] {
    outProtocol "can't open $n to write"
    return 0
  }
  return $f
}

proc getFileDialogTypes {ext} {
  global fileDialogSET
  if {$fileDialogSET == ""} {set fileDialogSET {{"All files" {*}}}}
  if {$ext != ""} {
    # reorder types to have the selected type first
    set count [llength $fileDialogSET]
    for {set i 0} {$i < $count} {incr i} {
      set line [lindex $fileDialogSET $i]
      foreach t [lindex $line 1] {
	if {$t != ".$ext"} continue
	set fset [list $line]
	for {set j 0} {$j < $count} {incr j} {
	  if {$j == $i} continue
	  lappend fset [lindex $fileDialogSET $j]
	}
	return $fset
      }
    }
  }
  return $fileDialogSET
}

proc fileDialog {operation {ext ""} {ifile Untitled}} {
  set types [getFileDialogTypes $ext]
  set def [entryVal defdirectory]
  if {$operation == "open"} {
    if {$def != ""} {
      return [tk_getOpenFile -filetypes $types -initialdir $def]
    }
    return [tk_getOpenFile -filetypes $types]
  }
  if {$ext == ""} {set ext txt}
  set ifile [file tail $ifile]
  if {$def != ""} {
    return [tk_getSaveFile -filetypes $types  \
		-initialfile $ifile \
		-defaultextension .$ext -initialdir $def]
  }
  return [tk_getSaveFile -filetypes $types  \
	      -initialfile $ifile -defaultextension .$ext]
}

proc getSerializeProc {ext} {
  set capvers "[string toupper [string range $ext 0 0]][string range $ext 1 2]"
  return [info commands serialize${capvers}File]
}

proc yscroll {w command {side right}} {
  global bgColor scrollWidth
  append w ".yscroll"
  scrollbar $w -command $command -bg $bgColor -width $scrollWidth
  pack $w -side $side -fill y
}

proc xscroll {w command {side top}} {
  global bgColor scrollWidth
  append w ".xscroll"
  scrollbar $w -command $command -bg $bgColor\
      -width $scrollWidth -orient horizontal
  pack $w -side $side -fill x
}

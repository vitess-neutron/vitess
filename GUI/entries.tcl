### project Xcontrol
### HMI DN
### M. Fromme
### June 1999

proc showRange {name e op} {
  if {$e == ""} { return ""}
  if [string match *\[a-z\]* $e] {
    set op [string range $e 0 1]
    set e  [string range $e 2 end]
  }
  return "\n\t    $name $op $e "
}

proc generateExplanation {w list {descr ""}} {
  regsub -all {[^].a-zA-Z0-9()[_-]} [lindex $list 0] " " explanation
  set more [lindex $list 1]
  global MaxOutstringLength Infolevel VisibleModule
  set mlen [expr $MaxOutstringLength - 8]
  if {$more != ""} {
    foreach s [split $more "\n"] {
      while {[string length $s] > $mlen} {
        set ts [string range $s 0 $mlen]
        if {[set j [string last " " $ts]] < 0} {
          append explanation "\n\t[string range $s 0 $mlen]"
          set j $mlen
        } else {
          append explanation "\n\t[string range $ts 0 $j]"
        }
        set s [string range $s [incr j] end]
      }
      append explanation "\n\t$s"
    }
  }

  if {$Infolevel == "expert"} {
    append explanation "\n\tinternal variable name : [lindex $descr 0]"
    switch [lindex $descr 1] {
      int - float {
        set arg4 [lindex $descr 4]
        set arg5 [lindex $descr 5]
        if {$arg4 == "1" && $arg5 == ""} {
          append explanation "\n\tmandatory;"
        } else {
          if {[lindex $descr 6] == "1"} {
            append explanation "\n\tmandatory; restrictions: "
          } else {
            append explanation "\n\trestrictions:"
          }
          set name [lindex $descr 0]
          append explanation [showRange $name $arg4 ge]
          append explanation [showRange $name $arg5 le]
        }
      }
    }
  }
  if {$explanation == ""} return
  if {[set opt [lindex $list 3]] != ""} {
    append explanation "\n\tcommand option -$opt"
  }
  if {[set callback [lindex $list 2]] == ""} {set callback showText}
  bind $w <ButtonPress> [list $callback $explanation]
  if {$opt == ""} return
  if [catch {set vi $VisibleModule}] {set vi 0}
  bind $w <ButtonPress> +[list addSeriesName $vi $opt [lindex $descr 0]]
}

proc boundLabel {w description lwidth} {
  set label [lindex $description 3]
  global labColor
  set t [lindex $label 0]
  # to print a lookalike for Angstroem : A with one . above
  #regsub -all {\[A\]} [lindex $label 0] \[\xc1\] t
  label $w -text [padS $t $lwidth] -font [labelFont] -bg $labColor
  generateExplanation $w $label $description
}

proc boundLabelA {w label lwidth} {
  global labColor
  label $w -text [padS [lindex $label 0] $lwidth]\
      -font [labelFont] -bg $labColor
  generateExplanation $w $label
}

proc moveLeft {w} {
  set i [$w index insert]
  if {[incr i -1] >= 0} {
    $w icursor $i
  }
}

proc moveRight {w} {
  set i [$w index insert]
  if {[incr i] <= [string length [$w get]]} {
    $w icursor $i
  }
}

### one entry in a row
###
proc myEntry {w variable width {app ""} {line ""}} {
  global entryColor
  if {$line != ""} {
    forceDef $variable$app [lindex $line 2]
  }
  if {$variable != ""} {
    entry $w -width $width -relief sunken -textvariable $variable$app -bg $entryColor
  } else {
    entry $w -width $width -relief sunken -bg $entryColor
  }
  if {$line != ""} {
    set com [string trim [list errorInLine $line $app]]
    bind $w <Return> $com
    bind $w <KP_Enter> $com
  }
}

proc optEntry {w var items} {
  global bgColor radioColor buttonColorFont
  set len 1
  foreach i $items {
    set ltmp [string length $i]
    if {$ltmp > $len } {
      set len $ltmp
    }
  }
  menubutton $w -textvariable $var -indicatoron 1 -menu $w.menu \
            -relief raised -bd 2 -highlightthickness 2 -anchor w -width $len \
            -direction flush -bg $radioColor -fg $buttonColorFont -font [textFont]
  menu $w.menu -tearoff 0
  foreach i $items {
    $w.menu add radiobutton -label $i -variable $var
  }
}

### entry with events bound to procedure
###

proc valEntryLabel {w variable label labelwidth width {app _}} {
  lFrame $w
  boundLabelA $w.l $label $labelwidth
  myEntry $w.e $variable $width $app
  pack  $w.l $w.e -side left -anchor w
}

proc fileEntry {w line labelwidth width {app _}} {
  global bgColor radioColor buttonColorFont FontSizeIndex tcl_platform
  lFrame $w
  set variable [lindex $line 0]
  forceDef $variable$app [lindex $line 2]
  if {[set ext [lindex $line 5]] == ""} {set ext txt}
  boundLabel $w.l $line $labelwidth
  myEntry $w.e $variable $width $app
  set dirtype [lindex $line 7]
  if {$dirtype != "d"} {set dirtype f}
  set entype ""
  set mondefault 0
  switch [lindex $line 1] {
    browsedir        {set dirtype d}
    editablefile     {set entype 0}
    parbrowsefile    {set dirtype p}
    pareditablefile  {set entype 1 ; set dirtype p}
    moneditablefile  {set entype 2 ; set dirtype p; set dim 1}
    mneditablefile   {set mondefault 0; set entype 2 ; set dirtype p; set dim 1}
    mon2editablefile {set entype 2 ; set dirtype p; set dim 2}
    mn2editablefile  {set mondefault 0; set entype 2 ; set dirtype p; set dim 2}
    montemplot       {set entype 3; set dirtype f; set dim 0}
  }

  # selectable monitor output
  if {$entype == 2} {
    set el [lindex [lindex $line 3] 2]
    if {$el == "n"} {set fel 0} else {set fel 1}
  }

  # reduced width to save place, use text length - 2
  if {$FontSizeIndex >= 1 || $tcl_platform(os) == "Darwin"} {
    set ww1 7
    set ww2 4
  } else {
    set ww1 5
    set ww2 2
  }
  set fnt [ssbuttonFont]
  button $w.b -text Browse -background $bgColor -width $ww1 -font $fnt\
      -command [list browseFile $variable$app open $dirtype $ext 1]

  if {$entype == 3} {
    button $w.p -text Plot -background $bgColor -width $ww2 -font $fnt\
          -command [list plotMonFile $variable $app]
    # template radio selection
    forceDef [set var ${variable}_o$app] -
    frame $w.u -bg $bgColor
    optEntry $w.u.r $var [concat - [getPlotApps]]
    pack $w.u.r
    label $w.lu -text using -bg $bgColor
    pack $w.l $w.e $w.b $w.p $w.lu $w.u -side left -anchor w
    return
  }

  if {$dirtype == "d"} {set tt NewDir} {set tt BrowseN}
  button $w.bn -text $tt -background $bgColor -width $ww1 -font $fnt\
      -command [list browseFile $variable$app write $dirtype $ext]

  if {$entype != ""} {
    button $w.x -text Edit -background $bgColor -width $ww2 -font $fnt\
        -command "editFile $variable $entype $ext $app"
    if {$entype < 2} {
      pack $w.l $w.e $w.b $w.bn $w.x -side left -anchor w
    } else {
      button $w.p -text Plot -background $bgColor -width $ww2 -font $fnt\
          -command [list plotMonFile $variable $app]
      # Autoplot selection
      # toggle has a special _r_app variable name
      forceDef [set var ${variable}_r$app] $mondefault
      checkbutton $w.r -text AutoPlot -variable $var -bg $bgColor
      # template radio selection
      set tlist [getPlotApps]
      if {[llength $tlist] > 0} {
        # selection has a special _o_app variable name
        forceDef [set var ${variable}_o$app] -
        frame $w.u -bg $bgColor
        # add a default - option as first choice
        # which means do not select a template as default
        optEntry $w.u.r $var [concat - $tlist]
        pack $w.u.r
        pack $w.l $w.e $w.b $w.bn $w.x $w.p $w.u $w.r -side left -anchor w
      } else {
        pack $w.l $w.e $w.b $w.bn $w.x $w.p $w.r -side left -anchor w
      }
    }
  } else {
    pack $w.l $w.e $w.b $w.bn -side left -anchor w
  }
}

### two entries in a row
###

proc twovalEntryLabel {w var1 var2 label1 label2 labelwidth ewidth {app _}} {
  lFrame $w
  boundLabelA $w.l1 $label1 $labelwidth
  boundLabelA $w.l2 $label2 $labelwidth
  myEntry $w.sp1 $var1 $ewidth $app
  myEntry $w.sp2 $var2 $ewidth $app
  pack  $w.l1 $w.sp1 $w.l2 $w.sp2 \
      -side left -fill both -anchor w
}

proc nvalEntryLabel {w list labelwidth ewidth {app _} {vt ""}} {
  lFrame $w
  set i 1
  foreach l $list {
    boundLabel $w.l$i $l $labelwidth
    set name [lindex $l 0]
    forceDef [set var $name$app] [lindex $l 2]
    if {$vt == ""} {
      myEntry $w.sp$i $name $ewidth $app
    } else {
      optEntry $w.sp$i $var [lindex $l 4]
    }
    pack $w.l$i $w.sp$i -side left -anchor w
    incr i
  }
}


### get value of global variable used as entry
###                these variables have a general appendix, defaulted to v
###                if variable has not been defined, return ""
proc entryVal {v {app _}} {
  upvar #0 $v$app locv
  if [info exists locv] { return $locv}
  return ""
}
### return value if global variable v exists (in a set of sets)
###     and is not empty string
###     if the variable is of type string or ..filename, then enclose it in
###     "" quotes
###                otherwise return 0
proc strEntryVal {v {app _}} {
  upvar #0 $v$app locv
  if {![info exists locv] ||
      "" == [set line [findControlvarLine $v]]} {
    return ""
  }
  switch [lindex $line 1] {
    string - longstring - filename - parfilename - editablefile - browsefile - browsedir -\
        pareditablefile - parbrowsefile -\
        moneditablefile - mon2editablefile - mneditablefile - mn2editablefile { return "\"$locv\""}
    default { return $locv}
  }
}
### return value if global variable v exists and is not empty string
###                otherwise return 0
proc logVal {v} {
  upvar #0 $v lv
  if [info exists lv] {
    if {$lv == ""} {return 0}
    return $lv
  }
  return 0
}

###
### row of radiobuttons
###    labels found from (list entry 3 of) description line of variable
###
### if we find too much items, or items with very long labels,
### we separate items in two or more rows
###
proc radioRowlpar {w line app {lwidth 12}} {
  global bgColor radioColor buttonColorFont
  set font [textFont]
  forceDef [set var [lindex $line 0]$app] [lindex $line 2]
  boundLabel $w.l $line $lwidth
  pack $w.l -side left -anchor w
  set i 0; set c 0; set testlen 0
  set w [frame $w.f -bg $bgColor]
  pack $w -side left
  frame $w.f$c -bg $bgColor
  pack $w.f$c -side top
  foreach f [lindex $line 4] {
    radiobutton $w.f$c.b$i -text $f -font $font \
        -variable $var -value $f -bg $radioColor -fg $buttonColorFont
    pack $w.f$c.b$i -in $w.f$c -padx 0.5m -side left -anchor w
    incr i
    incr testlen [expr 4 + [string length $f]]
    if {$testlen > 70} {
      incr c
      set testlen 0
      tFrame $w.f$c
    }
    incr i
  }
}

###
###  column of radiobuttons
###
proc radioRowlpar_down {w line app {lwidth 12}} {
  global radioColor buttonColorFont
  forceDef [set var [lindex $line 0]$app] [lindex $line 2]
  boundLabel $w.l $line $lwidth
  pack $w.l -side left -anchor nw
  set font [textFont]
  set i 0
  foreach f [lindex $line 4] {
    radiobutton $w.b$i -font $font \
        -text $f -variable $var -value $f -bg $radioColor -fg $buttonColorFont
    pack $w.b$i -in $w -padx 0.5m -side top -anchor w
    incr i
  }
}

### row of selectbuttons; labels found form argument list
###
proc selectRowlpar {w line app {lwidth 12}} {
  set font [stextFont]
  boundLabel $w.l $line $lwidth
  pack $w.l -in $w -side left -anchor w
  set v [lindex $line 0]
  set v [string tolower $v$app]
  global bgColor
  foreach f [lindex $line 4] {
    set text [lindex $f 0]
    set s [string tolower $text]
    set vv $v$s
    forceDef $vv [lindex $f 1]
    checkbutton $w.$s -text $text -variable $vv -font $font -background $bgColor
    pack $w.$s -in $w -side left -anchor w
  }
}


### define global variables if required (mode 1) or necessary
###   variables are taken from list of variable description lines/lists
###
proc setGlobalargs {mode args {app _}} {
  foreach a $args {
    set v [string tolower [lindex $a 0]]
    set t [lindex $a 1]
    if {$t == "" || $t == "header"} continue
    global $v$app
    if {$mode || ![info exists $v$app]} {
      set $v$app [lindex $a 2]
    }
  }
}

proc setGlobals {mode globvar {app _}} {
  foreach a [globVal $globvar] {
    set var [string tolower [lindex $a 0]]
    set t [lindex $a 1]
    if {$t == "" || $t == "header"} continue
    global $var$app
    if {$mode || ![info exists $var$app]} {
      set $var$app [lindex $a 2]
    }
  }
}

proc itemGroup {w iref inc it icount lit wid app {opt ""}} {
  global itemlabwidth
  upvar $iref i
  set nel [llength $lit]
  for {set li 0; set rest $it} {$rest > 0} {incr li $items} {
    set items [expr $rest >= $icount ? $icount : $rest]
    incr rest -$items
    set ni [expr $li + $items - 1]
    nvalEntryLabel $w.$i.e$opt [lrange $lit $li $ni] $itemlabwidth $wid $app $opt
    incr i $inc
  }
}


proc generateEntries {w globalset {delist {}} {app _}} {

  foreach l $delist {
    global $l
    unset $l;                                # unset old entry variables
  }
  global bgColor fileentrywidth itemlabwidth
  set gs [string trim $globalset]
  set all [globVal $gs]

  if {$globalset == "inputESET"} {
    # special layout with different entries in a row
    foreach i {0 1 2} {tFrame $w.$i}
    frame $w.3 -bg $bgColor
    pack $w.3 -side left -padx 1.4c
    frame $w.4 -bg $bgColor
    pack $w.4 -side left

    for {set i 0} {$i < 3} {incr i} {
      fileEntry $w.$i.e [lindex $all $i] $itemlabwidth $fileentrywidth $app
    }

    foreach i {3 4} o {"" opt} {
      nvalEntryLabel $w.3.e$o [list [lindex $all $i]] 1 8 $app $o
    }

    foreach i {5 6 7} o {"" opt t} {
      nvalEntryLabel $w.4.e$o [list [lindex $all $i]] 1 8 $app $o
    }
    return
  }

  set allitems [llength $all]
  for {set item 0} {$item < $allitems} {set item [incr k]} {
    set lintfloat {};    set leditfile {}
    set lfilestring {};  set llongstring {}; set lselect {}; set lradio {}
    set intfloat 0;      set filestring 0;   set longstring 0
    set editfile 0;      set radio 0;        set select 0
    set header ""

    for {set k $item} {$k < $allitems} {incr k} {
      switch [lindex [set line [lindex $all $k]] 1] {
        editablefile - browsefile - browsedir - pareditablefile - parbrowsefile -\
        moneditablefile - mon2editablefile - mneditablefile - mn2editablefile {
                           incr editfile;    lappend leditfile $line}
        string - filename {incr filestring;  lappend lfilestring $line}
        longstring        {incr longstring;  lappend llongstring $line}
        radio             {incr radio; lappend lradio $line}
        select            {incr select; lappend lselect $line}
        int - float       {incr intfloat;    lappend lintfloat $line}
        header            {
          set header [lindex $line 0]
          break
        }
        default           break
      }
    }
    if {$header == ""} {set total 0} else {set total 1}
    incr total [expr ($intfloat + 2)/3 + $editfile + \
                    ($radio + 2)/3 + $select + \
                    ($filestring + 1)/2 + $longstring]
    set i $item
    for {set j $i} {$j < $total + $i} {incr j} {
      tFrame $w.$j
    }

    foreach l $leditfile {
      fileEntry $w.$i.e $l $itemlabwidth $fileentrywidth $app
      incr i
    }

    itemGroup $w i 1 $filestring 2 $lfilestring 21 $app

    itemGroup $w i 1 $intfloat 3 $lintfloat 8 $app

    foreach l $llongstring {
      nvalEntryLabel $w.$i.el [list $l] $itemlabwidth $fileentrywidth $app
      incr i
    }

    itemGroup $w i 1 $radio 3 $lradio 8 $app opt

    foreach l $lselect {
      selectRowlpar $w.$i $l $app $itemlabwidth
      incr i
    }

    if {$header != ""} {
      sLabel $w.$i $header
      incr i
    }
  }
  propagateDigestValues
}

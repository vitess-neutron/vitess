### project Vitess
### HMI DN
### M. Fromme fromme@hmi.de
###
### procedures to define and use a digest of instrument parameters

# The "Instrument Digest" window shows principal input parameters
# of an instrument.
# These principal parameters, also noted as independent variables,
# may be free input parameters, or parameters chosen from modules of the instrument.
# Changing a principal parameter like the wavelength then
# directly modifies the according module parameter. 
# Other module parameters may be defined as dependent parameters,
# whose values are computed from independent parameters by arbitrary functions.

proc editFormula {w i} {
  # get formula from storage and put it to the edit region
  if {[set s [entryVal digestFormula$i]] == ""} {
    set s [entryVal digestDef$i]
  }
  $w.a.t delete 1.0 end
  $w.a.t insert end "$s\n"
}

proc storeFormula {w i} {
  # obtain formula from edit region and store it
  set s [$w.a.t get 1.0 end]
  if {$s == ""} return
  gSet digestFormula${i} $s
  regsub -all \n $s " " s
  regsub -all "  " $s " " s 
  gSet digestDef${i}_ [string trim $s]
}

proc checkFormula {com exp} {
  set s "${com}set __v \[expr $exp\]"
  if [catch {eval $s}] {return 0}
  return 1
}

proc propagateDigestValues {} {
  global digestSource digestTarget digestFormula digestIndepVarname

  if {![info exists digestSource]} return
  if {$digestSource == ""} return

  # Two loops for all digest variables:
  # first:  Get all indepent variable values and construct a set command;
  # second: Compute values by formulas and set values in digest window,
  #         propagate values to module variables if necessary.

  set setcom ""
  foreach indyname $digestIndepVarname source $digestSource {
    if {$indyname == ""} continue
    set v [globVal $source]
    append setcom "set $indyname {$v};"
  }

  foreach form $digestFormula source $digestSource target $digestTarget {
    if {$form == ""} {
      set v [globVal $source]
    } else {
      set s "${setcom}set v \[expr $form\]"
      if [catch {eval $s}] {set v ""}
      gSet $source $v
    }
    if {$target == ""} continue
    gSet $target $v
  }
}

#proc showGGG {} {
#  foreach gg [info globals *max_wave*] {
#    set v [globVal $gg]
#    puts "$gg: $v"
#  }
#}

proc definitionList {modnum name dform newname} {

  # obtain definition line for a variable and modify it for digest use.
  
  # we are sure that $name is part of module $modnum
  upvar #0 [globVal mod$modnum]ESET mod
  foreach line $mod {
    if {$name == [lindex $line 0]} break
  }

  set comment [lindex $line 3];		# get comment item

  # the visible label remains, but the auxilary comment is set differently
  set ins "this parameter sets $name of module $modnum"
  if {$dform != ""} {append ins "\ncomputed as $dform"}

  #            0:new name 1:old type     2: actual value
  #            3:new ins 
  #            rest: as before
  
  return [list $newname [lindex $line 1] [globVal ${name}_$modnum] \
	      [list [lindex $comment 0] $ins]\
	      [lrange $line 4 end]]
}

proc removeDigest {} {
  global digestSource Mlf Amf
  set digestSource {}
  foreach i {h digest} {catch {destroy $Amf.$i}}
  catch {destroy $Mlf.dig.b}
  moduleMenus
  helpFrame $Amf
}

proc digestFrame {w {app ""}} {
  # show digest parameters
  global digestFormula digestDefList bgColor

  set hfont [headerFont]
  fGroup $w.h $w.digest
  label $w.h.head -text "Instrument Digest" -font $hfont -fg steelblue -bg $bgColor
  pack $w.h.head

  set modl [itemize [entryVal moddigest]]
  set num [llength $modl]

  # sort independent and dependent parameters
  set indep 0
  set lindeps {} ; set ldeps {}
  foreach dl $digestDefList form $digestFormula {
    if {$form == ""} {
      incr indep
      lappend lindeps $dl
    } else {
      lappend ldeps $dl
    }
  }

  # frames for 2 header rows + indep. and dependent parameters
  set dep [expr $num - $indep]
  set indeprows [expr ($indep + 2) / 3]
  set deprows [expr ($dep + 2) / 3]

  set ww $w.digest
  set ffs [expr 1 + $indeprows]
  if {$deprows > 0} {set ffs [expr $ffs + 1 + $deprows]}

  for {set j 1} {$j <= $ffs} {incr j} {
    tFrame $ww.$j
  }

  set i 1
  if {$indeprows > 0} {
    itemGroup $ww i 1 $indep 3 $lindeps 8 $app
  }

  if {$deprows > 0} {
    label $ww.$i.l -text "Computed Parameters" -font $hfont -fg steelblue -bg $bgColor
    pack $ww.$i.l
    incr i
    itemGroup $ww i 1 $dep 3 $ldeps 8 $app
  }

  button $ww.$i.p -text "Propagate Values" -background $bgColor\
      -command propagateDigestValues
  button $ww.$i.r -text "Change Digest" -background $bgColor\
       -command genDigest
  pack $ww.$i.p -side left  -anchor s -pady 4
  pack $ww.$i.r -side right -anchor s -pady 4
}

proc showDigest {{w ""}} {
  global DummyEntry Amf
  if {$w == ""} {set w $Amf}

  upvar #0 VisibleModule vi
  upvar #0 visM$vi visible
  # delete whatever has been in this frame
  if {$visible == $DummyEntry} {set n $w.label} else {set n $w.$visible}
  catch {destroy $n}
  foreach i {h digest} {catch {destroy $w.$i}}
  set visible digest
  digestFrame $w
  moduleMenus
}

proc finalizeDigest {w} {
  set modl [itemize [entryVal moddigest]]
  set num [llength $modl]
  set testcom ""
  set vars {} ; set indep {} ; set depglobals {} ; set formulas {}
  for {set i 1} {$i <= $num} {incr i} {
    set s [entryVal digestDef$i]
    # distinguish between variables and expressions
    if [regexp {^[a-zA-Z][a-zA-Z0-9_]*$} $s] {
      lappend formulas ""
      lappend indep $s
      # hope that 1 is reasonable for independent variables
      append testcom "set $s 1;"
    } else {
      lappend formulas $s
    }
  }
  # check if independent variables are unique
  if {[llength $indep] != [llength [lsort -unique $indep]]} {
    showText "!duplicate variable names\n$indep"
    return
  }

  set nforms {} ; set testa {}
  foreach item $formulas {
    if {$item != ""} {
      # change variable names to variable references in formulas
      #   do not touch items with variable definitions
      if {[string first \$ $item] < 0} {
	foreach v $indep {
	  regsub -all \\m$v\\M $item {$\0} item
	}
      }
      lappend testa $item
    }
    lappend nforms $item
  }
  set formulas $nforms
      
  foreach t $testa {
    if [checkFormula $testcom $t] continue
    showText "!suspicious formula\n$t"
  }

  # find corresponding module variables
  global digestSource digestTarget digestFormula digestIndepVarname digestDefList Amf

  # start with empty lists
  set digestSource {}  ; set digestTarget {}
  set digestFormula {} ; set digestIndepVarname {}
  set digestDefList {}
  set i 1
  foreach m $modl {
    set mil [split $m ":"]
    set modnum [lindex $mil 1]
    set indepvname [entryVal digestDef$i]
    set name [lindex $mil 0]
    set newname digest_$name
    if {$modnum == ""} {
      # free variable
      set dtarget "" ; set dform ""
      set dlist [list $newname float "" [list $indepvname "independent digest variable"]]
    } else {
      # name gives module entry textvariable name
      set dtarget ${name}_$modnum
      if {[set dform [lindex $formulas [expr $i - 1]]] ne ""} {
	# value will not be used as formula input
	set indepvname ""
      }
      # construct definition list for later entry
      set dlist [definitionList $modnum $name $dform $newname]
    }
    lappend digestSource $newname
    lappend digestTarget $dtarget
    lappend digestFormula $dform
    lappend digestIndepVarname $indepvname
    lappend digestDefList $dlist
    
    incr i
  }
  #debug puts "digestDefList $digestDefList"
  #debug puts "digestSource $digestSource"
  #debug puts "digestTarget $digestTarget"
  #debug puts "digestFormula $digestFormula"
  #debug puts "digestIndepVarname $digestIndepVarname"

  showDigest;			# digest in actual module frame
  destroy $w;		        # destroy definiton window
}

proc inputDigest {w} {
  
  set modl [itemize [entryVal moddigest]]
  set ll [llength $modl]
  if {$ll <= 0} {
    showText "!Select parameters first"
    return
  }
  global entryColor labColor bgColor EntryCharWidth EntryCharHeight

  dialogSWindow $w "Define Instrument Digest"

  frame $w.t -bg $bgColor
  pack $w.t -side top -fill both -expand yes

  foreach f {a b} {
    frame $w.$f -bg $bgColor
    pack $w.$f -side top -fill x -expand no -padx 3 -pady 3
  }
  set lfont [labelFont]

  set ewid 8
  # find width and height of characters, somehow
  set swid [expr $ewid * $EntryCharWidth * 2.2]
  # 2 pixel distance to next entry.
  set shei [expr ($EntryCharHeight + 2)*($ll + 1)]
  set ww [scrollFrame $w.t right 8c 5c $shei $swid]

  set lwidth 20
  set i 1
  foreach m $modl {
    set g $ww.$i
    frame $g -bg $bgColor
    pack  $g -side top -fill both -pady 1
    set si digestDef${i}_
    # use given name for free variables and v$i otherwise
    if {[string first : $m] < 0} {
      gSet $si $m
    } elseif {[globVal $si] == ""} {
      gSet $si v$i
    }
    label $g.l -text [padS $m $lwidth] -font $lfont -bg $labColor
    entry $g.e -relief sunken -textvariable $si -bg $entryColor -width 8
    button $g.be -text edit -command [list editFormula $w $i] -background $bgColor
    button $g.bs -text "store formula" -command [list storeFormula $w $i]\
	-background $bgColor
    pack $g.l $g.e $g.be $g.bs -side left -anchor w -fill x -expand no -padx 3 -pady 3
    incr i
  }
  # text area to edit parameter formula
  set ww $w.a
  text $ww.t -relief raised -bd 2 \
      -height 4 -width 64\
      -wrap none\
      -relief sunken\
      -font [textFont]\
      -setgrid 1\
      -yscrollcommand "$ww.yscroll set" -bg $bgColor
  yscroll $ww "$ww.t yview"
  pack $ww.t -side left -expand yes -fill both
  
  set ww $w.b
  bButton $ww.b << genDigest
  bButton $ww.s >> "finalizeDigest $w"
  bButton $ww.c Cancel "destroy $w"
  pack $ww.s -side right -anchor w
  pack $ww.b -side left -anchor w
  pack $ww.c -side top -anchor w
}

proc genDigest {{w .gdig}} {

  dialogSWindow $w "Define Instrument Digest" "+100+640"

  global entryColor labColor bgColor EntryCharWidth EntryCharHeight
  foreach f {h1 h2 h3 w b} {
    frame $w.$f -bg $bgColor
    pack $w.$f -side top -fill x -expand no -padx 3 -pady 3
  }

  set lfont [labelFont]

  label $w.h1.l  -font $lfont -bg $labColor -text \
"space separated list of Name:Moduleand free variable items e.g.
  wavelength min_width:1 max_width:1
where wavelength is a free variable, and the next two belong to module 1
"
  pack $w.h1.l -side left -anchor w
  set ewid 80
  entry $w.h2.e -width $ewid -relief sunken -textvariable moddigest_ -bg $entryColor\
      -xscrollcommand "$w.h3.xscroll set"
  pack $w.h2.e -side left -anchor w
  # Compute pixel size of entries, to be of use for scrollregion calculations.
  set EntryCharWidth [expr [winfo reqwidth $w.h2.e] / $ewid]
  set EntryCharHeight [winfo reqheight $w.h2.e]

  xscroll $w.h3 "$w.h2.e xview"

  bButton $w.b.c Cancel "destroy $w"
  bButton $w.b.n >> "inputDigest $w"
  pack $w.b.c -side left -anchor w
  pack $w.b.n -side right -anchor w
}

proc digestView {} {
  if {"" != [globVal digestSource]} {
    showDigest
  } else {
    genDigest
  }
}

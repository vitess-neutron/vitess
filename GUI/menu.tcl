### project Xcontrol
### HMI DN
### M. Fromme  
### June 1999

### tools to generate the motif like menu bar and pop up menues
###
### menu button
###
proc mButton {w text {bitm 0}} {
  global menuColor menuButtonColor
  if {$bitm != 0} {
    menubutton $w -text $text -font [buttonFont] \
	-relief raised -menu $w.menu -bitmap $bitm -bg $menuButtonColor
  } else {
    menubutton $w -text $text -font [buttonFont] \
	-relief raised -menu $w.menu -bg $menuButtonColor
  }
  menu $w.menu -bg $menuColor
}
###
### menu
###
proc mMenu {w text {tear 0} {n 0}} {
  global menuColor menuButtonColor
  menubutton $w -text $text -font [menubarFont] \
      -underline $n -menu $w.menu -bg $menuButtonColor
  menu $w.menu -bg $menuColor -tearoff $tear
}
### add a cascade entry
###
proc cascEntry {w var val} {
  global radioColor
  $w add radiobutton -variable $var -label $val -value $val -bg $radioColor
}
### add cascade of radiobutton given by global variable
###
proc cascEntries {w var args} {
  global menuColor
  menu $w -bg $menuColor -tearoff 0
  foreach val $args {
    $w add radiobutton -variable $var -label $val -value $val
  }
}

### present a popup menu to select a font
###
proc fontMenu {window var} {
  global menuColor
  set w $window.$var
  menu $w -bg $menuColor -tearoff 0
  $w add cascade -label Family -menu $w.family
  $w add cascade -label Size   -menu $w.size
  $w add cascade -label Type   -menu $w.type

  cascEntries $w.family ${var}family Helvetica Arial "Lucida Console" Courier Times
  cascEntries $w.size   ${var}size   8 9 10 11 12 13 14 18
  cascEntries $w.type   ${var}type   normal bold
}

### build a popup menu
###
proc popMenu {w args} {
  set f [menubarFont]
  switch [getSystem] {
    unix {set c separator}
    default {set c "command -label \"- - - - - - - - - - -\" -font \$f"}
  }
  foreach a $args {
    set l [lindex $a 1]
    set m [lindex $a 2]
    switch [lindex $a 0] {
      c {$w add command -font $f -label $l -command $m}
      s {eval $w add $c}
      S {$w add command -label $l -font $f }
      m {$w add cascade -font $f -label $l -menu $w.$m}
    }
  }
}

proc getFreePlot {} {
  global Plotindex
  if [catch {incr Plotindex}] {
    set Plotindex 1
  }
  return $Plotindex
}


proc readXYZFile {f_i rows_i cols_i xl_i yl_i a_i} {
  upvar $f_i f
  upvar $xl_i xl
  upvar $yl_i yl
  upvar $rows_i rows
  upvar $cols_i cols
  upvar $a_i a
  while { ! [eof $f] } {
    if {[gets $f ins] <= 0} continue
    set ll [eval list $ins]
    set x [expr 0 + [lindex $ll 0]]
    if {$x == ""} continue
    set y [expr 0 + [lindex $ll 1]]
    set arr($x,$y) [expr 0 + [lindex $ll 2]]
    lappend xl $x
    lappend yl $y
  }
  set xl [lsort -real -unique $xl]
  set cols [llength $xl]
  set yl [lsort -real -unique $yl]
  set rows [llength $yl]
  # prepare value rows, from top to bottom
  foreach y $yl {
    set xa {}
    foreach x $xl {
      if [catch {set v $arr($x,$y)}] {set v 0}
      lappend xa $v
    }
    lappend a $xa
  }
}

proc checkPlotfile  {fname} {
  if [catch {open $fname r} f] {
    showText "! can't open $fname"
    return 0
  }
  if [eof $f] {
    close $f
    showText "! empty $fname"
    return 0
  }
  gets $f ins
  close $f
  if {2 > [scan $ins "%f%f%f%f" x y xe ye]} {
    showText "! insufficient plot file"
    return 0
  }
  return 1
}

# show 2d array coded with colors
proc show2Dfile {fname} {

  if {! [checkPlotfile $fname]} return

  set f [open $fname r]

  set i [getFreePlot]
  set w .plot$i

  set rows 1
  set a {};      # list of row lists, top to bottom
  set xl {};	 # x tic values
  set yl {};     # y tic values

  gets $f ins

  set ll [eval list $ins]
  if [string compare "#x y z" "$ll"] {
    set xl $ll;	# first line and first column are tic values
    while {[gets $f ins] > 0} {
      incr rows
      set ll [eval list $ins]
      lappend yl [lindex $ll 0]
      lappend a [set inp [lrange $ll 1 end]]
      if {$rows == 2} {set cols [llength $inp]}
    }
  } else {
    readXYZFile f rows cols xl yl a
  }
  close $f

  # find min, max
  set min [set max [lindex [lindex $a 0] 0]]
  foreach row $a {
    foreach v $row {
      if {$v > $max} {set max $v} elseif {$v < $min} {set min $v}
    }
  }

  if {$max == $min} {set factor 1} else {
    set factor [expr 255 / ($max - $min)]
  }
  set canvaswidth 10.0
  set canvasheight [expr $canvaswidth * $rows / $cols]
  catch {destroy $w}
  global bgColor
  toplevel $w -background $bgColor
  wm title $w [set tit "Y-Z Plot [file tail $fname]"]
  wm iconname $w $tit
  dismissFrame $w

  set lpos 0.8c
  set mpos "[expr $canvaswidth / 2]c"
  set rpos "[expr $canvaswidth - 1]c"

  # display area for values
  set c $w.c
  canvas $c -width ${canvaswidth}c \
      -height "[expr $canvasheight + 1]c" -bg $bgColor
  pack $c
  set lowc "[expr $canvasheight + 0.2]c"
  set botc "[expr $canvasheight + 0.6]c"
  $c create text $lpos $lowc -anchor n -text [format %8.3g [lindex $xl 0]]
  $c create text $mpos $lowc -anchor n -text horizontal
  $c create text $rpos $lowc -anchor n -text [format %8.3g [lindex $xl end]]
  $c create text $lpos $botc -anchor n -text [format %8.3g [lindex $yl 0]]
  $c create text $mpos $botc -anchor n -text vertical
  $c create text $rpos $botc -anchor n -text [format %8.3g [lindex $yl end]]

  # color bar
  $c create rectangle 0 0 ${canvaswidth}c ${canvasheight}c\
      -fill black -outline gray
  set cc $w.cc
  set hc 0.3c
  canvas $cc -width ${canvaswidth}c -height 0.8c -bg $bgColor
  pack $cc -pady $hc
  $cc create text $lpos 0.4c -anchor n -text [format %8.3g $min]
  $cc create text $mpos 0.4c -anchor n -text "value range"
  $cc create text $rpos 0.4c -anchor n -text [format %8.3g $max]

  set colist {}
  set x1 0
  set x2 [set xdelta [expr $canvaswidth / 256]]
  for {set i 0} {$i < 256} {incr i} {
    lappend colist [set v [format "#%02x0000" $i]]
    $cc create rectangle ${x1}c 0 ${x2}c $hc -fill $v -outline ""
    set x1 $x2
    set x2 [expr $x1 + $xdelta]
  }

  # plot data as colored squares
  set xdelta [expr $canvaswidth / $cols]
  set ydelta [expr $canvasheight / $rows]
  set y2 $canvasheight
  set y1 [expr $y2 - $ydelta]
  foreach row $a {
    set x1 0
    set x2 $xdelta
    foreach v $row {
      set rc [expr round(($v - $min) * $factor)]
      if {$rc > 0} {
	$c create rectangle ${x1}c ${y1}c ${x2}c ${y2}c\
	    -fill [lindex $colist $rc] -outline ""
      }
      set x1 $x2
      set x2 [expr $x1 + $xdelta]
    }
    set y2 $y1
    set y1 [expr $y2 - $ydelta]
  }
}

###
### plotMonFile
proc plotMonFile {type v app} {
  set fn [file join [entryVal defdirectory] [entryVal $v $app]]
  if {$type == 2} {show2Dfile $fn} else {showXYfile $fn}
}

###
### plotFile
proc plotFile {{twod 0}} {
  catch {fileDialog open} name
  if {$name == ""} return
  switch $twod {
    1 { showXYfile $name}
    2 { show2Dfile $name}
  }
}

proc plotCmdWindow {} {
  set w .x.plotcmd
  generateToplevel $w "Gnuplot Command" "" +20+[expr [winfo screenheight .] - 80]
  global entryColor GnuPlotCmd buttonColor
  forceDef GnuPlotCmd ""
  entry $w.e -width 120 -relief sunken -textvariable GnuPlotCmd -bg $entryColor
  set com "doGnuplotCmd $w"
  bind $w.e <Return> $com
  bind $w.e <KP_Enter> $com
  button $w.do -text Do -command $com -font [sbuttonFont] -background $buttonColor
  pack $w.e -side left -expand no
  pack $w.do -side left
}

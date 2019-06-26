### Project Vitess
### plot graphs with plain TclTK widgets

proc pCompare {a b} {
  set xa [lindex $a 0]
  set xb [lindex $b 0]
  if {$xa < $xb} { return -1}
  if {$xa == $xb} { return 0}
  return 1
}

proc showXYfile {fname} {
  if [catch {open $fname r} f] {
    showText "! can't open $fname"
    return
  }
  if [eof $f] {
    close $f
    showText "! empty $fname"
    return
  }

  set i [getFreePlot]
  set w .plot$i

  set p {}
  set xmin 1.0e24
  set xmax -1.0e24
  set ymin $xmin
  set ymax $xmax
  while {[gets $f ins] > 0} {
    if {2 == [scan $ins "%f%f" x y]} {
      if {$x < $xmin} {set xmin $x}
      if {$x > $xmax} {set xmax $x}
      if {$y < $ymin} {set ymin $y}
      if {$y > $ymax} {set ymax $y}
      lappend p [list $x $y]
    }
  }
  close $f
  set p [lsort -command pCompare $p]

  set canvaswidth 600
  set canvasheight 300
  catch {destroy $w}
  if {![generateToplevel $w "X-Y Plot"]} return

  set margin 10
  canvas $w.c -width $canvaswidth -height $canvasheight
  pack $w.c

  dismissFrame $w

  set c $w.c
  set x0 30
  set xf [expr $xmax - $xmin]
  if {$xf > 0} {
    set xf [expr ($canvaswidth - $margin - $x0) / $xf]
  } else {
    set xf 1
  }
  set y0 [expr $canvasheight - $x0]
  set yf [expr $ymax - $ymin]
  if {$yf > 0} {
    set yf [expr ($y0 - $margin) / $yf]
  } else {
    set yf 1
  }
  # y axis
  $c create line $x0 $y0 $x0 $margin -width 1
  # x axis
  $c create line $x0 $y0 [expr $canvaswidth - $margin] $y0 -width 1

  set lp {}
  foreach point $p {
    set x [lindex $point 0]
    set y [lindex $point 1]
    set x [expr round($x0 + $xf * ($x - $xmin))]
    set y [expr round($y0 - $yf * ($y - $ymin))]
    lappend lp $x $y
    $c create oval [expr $x-2] [expr $y-2] [expr $x+2] [expr $y+2] \
	-width 1 -outline black -fill SkyBlue2
  }
  eval $c create line $lp -fill #ff0000
}

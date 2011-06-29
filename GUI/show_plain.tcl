### Project VITESS
### plot graphs with plain TclTK widgets

# M. Fromme, HZB June 2011
# some code for drawing axes has been adopted from emu-graph

## nicenum translated from C-code in Blt there, who got it from:
##      Taken from Paul Heckbert's "Nice Numbers for Graph Labels" in
##      Graphics Gems (pp 61-63).  Finds a "nice" number approximately
##      equal to x.

proc nicenum {x floor} {

  if {$x == 0} {return 0}

  set negative 0
  if {$x < 0} {
    set x [expr -$x]
    set negative 1
  }

  set exponX [expr floor(log10($x))]
  set fractX [expr $x/pow(10,$exponX)]; # between 1 and 10
  if {$floor} {
    if {$fractX < 1.5} {
      set nf 1.0
    } elseif {$fractX < 3.0} {
      set nf 2.0
    } elseif {$fractX < 7.0} {
      set nf 5.0
    } else {
      set nf 10.0
    }
  } elseif {$fractX <= 1.0} {
    set nf 1.0
  } elseif {$fractX <= 2.0} {
    set nf 2.0
  } elseif {$fractX <= 5.0} {
    set nf 5.0
  } else {
    set nf 10.0
  }
  if {$negative} {
    return [expr -$nf * pow(10,$exponX)]
  }
  set value [expr $nf * pow(10,$exponX)]
  return $value
}

proc y2canvas {g y} {
  global Egr
  return [expr int(($Egr($g,ymax) - $y) * $Egr($g,yfactor) + $Egr($g,ytop))]
}

proc x2canvas {g x} {
  global Egr
  return [expr int(($x - $Egr($g,xmin)) * $Egr($g,xfactor) + $Egr($g,xref))]
}

proc pCompare {a b} {
  set xa [lindex $a 0]
  set xb [lindex $b 0]
  if {$xa < $xb} {return -1}
  if {$xa == $xb} {return 0}
  return 1
}

proc prettyNumber {t} {
  return [format %g $t]
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

  # input data may not have ordered x values: 
  # sort them so that we may connect them by a graph line
  set p [lsort -command pCompare $p]

  set graph $fname
  global Egr bfontfamily lfontsize FontSizeIndex

  set Egr($graph,xmin) $xmin
  set Egr($graph,ymin) $ymin
  set Egr($graph,xmax) $xmax
  set Egr($graph,ymax) $ymax

  set canvaswidth 600
  set canvasheight 300
  catch {destroy $w}
  if {![generateToplevel $w "X-Y Plot"]} return

  canvas $w.c -width $canvaswidth -height $canvasheight
  pack $w.c

  printFrame $w

  set c $w.c

  $c create text 3c 1c -text "file $fname" -anchor sw

  # x0 is the left margin and position of y axis
  #   it should be big enough to allow axis labels left to to the y axis
  if {$FontSizeIndex > 0} {set sms 1} else {set sms 3}
  set yfont [list $bfontfamily [expr $lfontsize - $sms] normal]
  set x0 [font measure $yfont 123456789012]
  set Egr($graph,xref) $x0
  # xright is the right margin of the data rectangle
  set xright 8
  # yref is the bottom margin
  set Egr($graph,yref) [set ymargin 30]
  # ytop is the top margin
  set Egr($graph,ytop) [set ytop 8]
  
  # y0 is the canvas y coordinate of the x axis 
  set y0 [expr $canvasheight - $ymargin + $ytop]

  # value ranges of data
  set xdelta [expr double($xmax - $xmin)]
  set ydelta [expr double($ymax - $ymin)]
  if {$xdelta == 0} {set xdelta 0.001}
  if {$ydelta == 0} {set ydelta 0.001}

  # factors are used to scale user to canvas coordinates
  set Egr($graph,xfactor) [expr double($canvaswidth - $x0 - $xright)/$xdelta]
  set Egr($graph,yfactor) [expr double($y0 - $ytop)/$ydelta]

  # parameters for axis drawing
  set ticklen 4
  set axistextoffset 5
  set nticks_x 6
  set nticks_y 5

  # y-pos of tick end points and of axis tick labels
  set ticky [expr $y0 - $ticklen]
  set texty [expr $y0 + $axistextoffset]
  # put ticks and numbers on the axis
  # starting at next nice number above xmin
  set delta_x [nicenum [expr double($xmax - $xmin)/$nticks_x] 1]
  set nicex_min [nicenum $xmin 1]

  # x axis
  $c create line $x0 $y0 [expr $canvaswidth - $xright] $y0 -width 1

  set xfont [list $bfontfamily [expr $lfontsize - $sms + 1] bold]
  for {set t $nicex_min} {$t <= $xmax} {set t [expr $t+$delta_x]} {
    if {$t >= $xmin} {
      set x [x2canvas $graph $t]
      # tick on x axis
      $c create line $x $y0 $x $ticky
      # tick on top
      $c create line $x $ytop $x [expr $ytop + $ticklen]
      # add the label
      $c create text [x2canvas $graph $t] $texty -text [prettyNumber $t] -font $xfont -anchor n
    }
  }

  # y axis
  $c create line $x0 $y0 $x0 $ytop -width 1

  # x0 is the canvas x position of the y axis
  # xrpos is the canvas x position of the right edge
  set xrpos [x2canvas $graph $xmax]
  set tickx1 [expr $x0 + $ticklen]
  set tickx2 [expr $xrpos - $ticklen]
  set textx  [expr $x0 - $axistextoffset]

  set nicey_min [nicenum $ymin 1]
  set delta_y [nicenum [expr double($ymax - $ymin)/$nticks_y] 1]

  for {set f $nicey_min } {$f <= $ymax} {set f [expr $f + $delta_y]} {
    if {$f >= $ymin} {
      set y [y2canvas $graph $f]
      $c create line $x0 $y $tickx1 $y
      $c create line $xrpos $y $tickx2 $y
      # add the label
      $c create text $textx $y -text [prettyNumber $f] -anchor e -font $yfont
    }
  }

  # draw data points
  set lp {}
  foreach point $p {
    set x [x2canvas $graph [lindex $point 0]]
    set y [y2canvas $graph [lindex $point 1]]
    lappend lp $x $y
    $c create oval [expr $x-2] [expr $y-2] [expr $x+2] [expr $y+2] \
	-width 1 -outline black -fill SkyBlue2
  }
  # data points connected by a red line
  eval $c create line $lp -fill #ff0000
}

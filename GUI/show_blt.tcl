### Project Vitess
### plot graphs using the BLT::graph widget

package require BLT
if { $tcl_version >= 8.0 } {
  catch {
    namespace import blt::*
    namespace import -force blt::tile::*
  }
}

set GraphOptions {
  Font		        {Helvetica 9 normal}
  Legend.hide           true
  background		white
  x.Loose		no
  x.Title		X
  y.Rotate		90
  y.Title		Y
}

proc showDPoint {w x y} {
  if [catch { $w element closest $x $y e -halo 2c -interpolate yes } r] return
  if {!$r} return
  regsub .graph $w "" a
  regsub .plot $a "" a
  if {$e(name) != "line$a"} return
  catch {
    upvar #0 VX$a GX
    upvar #0 VY$a GY
    set i $e(index)
    set fx [lindex $GX $i]
    set fy [lindex $GY $i]
    upvar #0 GP$a Gpostext
    set Gpostext "x: [format %8.3g $fx]  y: [format %8.3g $fy]"
  }
}

proc showXYfile {fname} {
  if {[catch {open $fname r} f] || [eof $f]} {
    showText "! can't open $fname"
    return
  }
  global GraphOptions plotmode buttonColor bgColor scrollWidth

  set i [getFreePlot]
  set w .plot$i
  
  upvar #0 VX$i GX
  upvar #0 VY$i GY
  set postextname GP$i

  catch {vector destroy VX VY}
  vector create VX
  vector create VY

  while {[gets $f ins] > 0} {
    if {2 != [scan $ins "%f%f" x y]} continue
    VX append $x
    VY append $y
  }
  close $f
  VX sort VY

  # extract values as list in global variable
  set GX [VX range 0 end]
  set GY [VY range 0 end]
  catch {vector destroy VX VY}

  catch {destroy $w}
  toplevel $w -background $bgColor
  set g $w.graph
  set resource [string trimleft $g .]
  foreach {option value} $GraphOptions {
    option add *$resource.$option $value
  }
  option add *$resource.Title $fname

  graph $g -width 20c -height 12c -bd 0

  if {$plotmode == "dots"} {set p 0} else {set p 1}
  $g pen create myPen -color red4 -fill red1 -symbol circle\
      -pixels 1m -linewidth $p

  $g element create line$i -xdata $GX -ydata $GY -pen myPen
  Blt_ZoomStack $g

  $g postscript configure \
      -center yes -maxpect yes -landscape no -preview yes

  scrollbar $w.xbar -command "$g axis view x" -orient horizontal -bg $bgColor -width $scrollWidth
  scrollbar $w.ybar -command "$g axis view y" -orient vertical -bg $bgColor -width $scrollWidth
  $g axis configure x -scrollcommand "$w.xbar set"
  $g axis configure y -scrollcommand "$w.ybar set"

  $g element bind all <Enter> { showDPoint %W %x %y }

  set bfont [buttonFont]
  htext $w.footer -cursor arrow -background $bgColor -text {%%
button $htext(widget).quit -text dismiss -command "destroy $w" -font $bfont -background $buttonColor
$htext(widget) append $htext(widget).quit
%%                      print to file xy.ps %%
button $htext(widget).print -text print -command "catch \{$g postscript output xy.ps\}" -font $bfont -background $buttonColor
$htext(widget) append $htext(widget).print
%%           %%
label $htext(widget).pos -textvariable $postextname
$htext(widget) append $htext(widget).pos
%%
}

  table $w \
      0,0 $g -fill both -cspan 3 -rspan 3 \
      0,3 $w.ybar -fill y -padx 0 -pady 0 -rspan 3 \
      3,0 $w.xbar -fill x -cspan 3 -padx 0 -pady 0\
      4,0 $w.footer -cspan 4 -fill x
}

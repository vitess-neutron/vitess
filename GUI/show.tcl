proc getFreePlot {} {
  global Plotindex
  if [catch {incr Plotindex}] {
    set Plotindex 1
  }
  return $Plotindex
}

# show 2d array coded with colors
proc show2Dfile {fname} {
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

  set rows 0
  set a {}
  set xl {};				# x tic values
  set yl {};				# y tic values

  while {[gets $f ins] > 0} {
    incr rows
    set ll [eval list $ins]
    if {$rows == 1} {
      set xl $ll
      continue;				# first line and first column are tic values
    }
    lappend yl [lindex $ll 0]
    set inp [lrange $ll 1 end]
    if {$rows == 2} {
      set cols [llength $inp]
      set max [set min [lindex $inp 0]]
    }
    lappend a $inp
    foreach v $inp {
      if {$v > $max} {set max $v} elseif {$v < $min} {set min $v}
    }
  }
  close $f
  if {$rows == 0} {
    showText "! empty file $fname"
    return
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
    2 { show2Dfile $name}
    1 { showXYfile $name}
    0 {
      global plotapp_ tcl_platform
      if {$tcl_platform(platform) == "windows"} {
	if [catch {glob $plotapp_} resname] {
	  showText "!No valid plot application specified"
	  return
	}
	set plotapp_ $resname
      }
      set tname [tmpFilename tplot.dat]
      if [catch {open $tname w} f] return
      puts $f "plot '$name'\npause -1\n"
      close $f
      catch {exec 2> /dev/null $plotapp_ $tname &}
      catch {file delete $tname}
    }
  }
}

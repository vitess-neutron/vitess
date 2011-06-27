### project Xcontrol
### HMI DN
### M. Fromme fromme@hmi.de
### June 1999

###
### window tools
###

### scrollable text window
###
proc textWindow {w th tfont {series 0}} {
  global bgColor MaxOutstringLength Textw LastMarker

  frame $w.b -relief sunken -bd 2 -bg $bgColor
  frame $w.a -relief sunken -bd 2
  frame $w.ax
  pack $w.b -side left
  pack $w.a -side top -fill both -expand yes
  bButton $w.b.c Clear clearText
  bButton $w.b.s Save saveText
  if {$w == "$Textw"} {
    bButton $w.b.b Big {sizeTextWindow 1}
  } else {
    if $series {
      bButton $w.b.b "Stop\nSeries" stopSeriesExecution
      bind $w.b.b <Destroy> "stopSeriesExecution ; sizeTextWindow"
    } else {
      bButton $w.b.b Small sizeTextWindow
      bind $w.b.b <Destroy> sizeTextWindow
    }
  }
  pack $w.ax $w.b.b $w.b.c $w.b.s -side top -fill x
  text $w.a.t -relief raised -bd 2 \
      -height $th -width [expr int(0.9*$MaxOutstringLength)]\
      -wrap none\
      -font $tfont\
      -setgrid 1\
      -xscrollcommand "$w.ax.xscroll set"\
      -yscrollcommand "$w.a.yscroll set" -bg $bgColor
  yscroll $w.a "$w.a.t yview"
  pack $w.a.t -side left -expand yes -fill both
  xscroll $w.ax "$w.a.t xview"
  set LastMarker [$w.a.t index "end - 1 lines"]
}

# Show output window either as part of main window(small) or as
# separate window(big).
#
proc sizeTextWindow {{bigwin 0} {series 0}} {
  global Textw Bigw Tth bgColor tfontfamily tfontsize tfonttype scrollWidth LastMarker FontSizeIndex
  upvar #0 Messagew w
  set fontsize $tfontsize
  set scrollWidth 8
  if $bigwin {
    set dw $Textw
    set gw $dw
    set Bigw .message
    generateToplevel $Bigw "VITESS Output"
    set w $Bigw.t
    if {$FontSizeIndex >= 1} {
      set th 30
      incr fontsize 1
    } else {
      set th 40
      incr fontsize 2
    }
    incr scrollWidth 4
  } else {
    set dw $Bigw
    set gw $Bigw.t
    set w $Textw
    set th $Tth
  }
  set s [$gw.a.t get 1.0 end]
  destroy $dw
  if [catch {frame $w -bg $bgColor}] return
  pack $w -side top -fill both -expand yes
  textWindow $w $th [list $tfontfamily $fontsize $tfonttype] $series
  if {$s == ""} return
  $w.a.t insert end "$s\n"
  set LastMarker [$w.a.t index "end - 1 lines"]
}

proc forceSmallTextWindow {} {
  set w [globVal Bigw]
  if {$w == ""} return
  if [winfo exists $w] sizeTextWindow
}


### insert text at end of text window
###
proc showText {s {newl \n} {errchar "!"}} {
  upvar #0 Messagew w
  if {$w == ""} return
  global LastMarker
  set wt $w.a.t;		# $w.a.t is text area
  set mode 0
  set anf [string range $s 0 2]
  if {$anf == "BBB"} {
    set s [string range $s 3 end]
    set mode 2
  } elseif {$anf == "RRR"} {
    set s [string range $s 3 end]
    set mode 3
  } elseif {[string index $s 0] == $errchar} {
    set s [string range $s 1 end]
    set mode 1
  }
  $wt insert end $s$newl
  set height [lindex [$wt configure -height] 4]
  # dmf: vielleicht noch etwas tiefer gehen ?
  $wt yview "end - $height lines"

  set ee [$wt index "end - 1 lines"]
  if {$mode > 0} {
    if {$mode == 3} {
      $wt tag add big3 $LastMarker $ee
      $wt tag configure big3 -relief sunken -foreground red -background white
    } else {
      $wt tag add big $LastMarker $ee
      $wt tag configure big -relief sunken -background white
    }
  }
  set LastMarker $ee

  global audible_bell
  if {$mode == 1 && $audible_bell} bell
  return $s
}

proc clearText {{s ""}} {
  zeroProgress
  upvar #0 Messagew w
  if {$w == ""} return
  set wt $w.a.t;		# $w.a.t is text area
  $wt delete 1.0 end
  if {$s == ""} return
  $wt insert end "$s\n"
}

proc saveText {} {
  upvar #0 Messagew w
  if {$w == ""} return
  set wt $w.a.t;		# $w.a.t is text area
  set s [$wt get 1.0 end]
  if {$s == ""} return
  if {0 == [set f [openWriteFile txt]]} return
  puts $f $s
  close $f
}

### get a free place on used X display with regard to toplevel tcl/tk
### windows
###
###
### get toplevel windows
###
proc getToplevelWindows {} {
  set list {}
  foreach i [info commands .*] {
    if {[winfo ismapped $i] && [winfo toplevel $i] == "$i"} {
      lappend list $i
    }
  }
  return $list
}


proc doesCut {x w y h xl xh yl yh} {
  return [expr $x < $xh && $x + $w > $xl && \
      $y < $yh && $y + $h > $yl]
}

proc testCorner {resx resy a b width height maxx maxy n rectangles} {
  upvar $resx resultx
  upvar $resy resulty
  upvar $rectangles r
  for {set j 0} {$j < $n} {incr j} {
    set x $r($j,$a)
    if {$x + $width > $maxx} continue
    set y $r($j,$b)
    if {$y < 0} {set y 0}
    if {$y + $height > $maxy} continue

    # the rectangle is free, if it doesn't cut any other rectangle
    for {set i 0} {$i < $n} {incr i} {
      if {[doesCut $x $width $y $height \
	  $r($i,xl) $r($i,xh) $r($i,yl) $r($i,yh)]} {
	break
      }
    }
    if {$i == $n} {
      # all checks done, return position
      set resultx $x
      set resulty $y
      return 1
    }
  }
  return 0
}

###
### Get a free rectangle on screen with respect to toplevel windows
### of the Tcl/Tk application.
### We use wm to obtain the actual position of a visible window, including the
### window manager generated frame.
### winfo is used to obtain the window size in pixels.
### The window manager frame is assumed to be motif - like.
###
proc getFree {{xwidth 400} {yheight 200}} {
  set i 0
  foreach w [getToplevelWindows] {
    set sform "%dx%d+%d+%d"
    if [winfo ismapped $w] {
      if {4 != [scan [winfo geometry $w] $sform width height winfox winfoy]} {
	# window system is not cooperative
	puts "$w : [winfo geometry $w]\n"
	return "+0+50"
      }
      if {4 != [scan [wm geometry $w] $sform wmwidth wmheight x y]} {
	# no window manager present, return dummy value
	return "+0+50"
      }
      # get offset by border frames generated from window manager
      set sidemargin [expr $winfox - $x]
      set topmargin [expr $winfoy - $y]

      set r($i,xl) $x
      set r($i,yl) $y
      set r($i,xh) [expr $x + 2 * $sidemargin + $width]
      set r($i,yh) [expr $y + $topmargin + $sidemargin + $height]
      incr i
    }
  }

  if {$i == 0} {
    return "+0+50"
  }

  set maxx [winfo screenwidth .]
  set maxy [winfo screenheight .]

  #first attached at (bottom,left)
  if [testCorner rx ry xl yh $xwidth $yheight $maxx $maxy $i r] {
    return "+$rx+$ry"
  }

  #then from left to right attached at (top,right)
  if [testCorner rx ry xh yl $xwidth $yheight $maxx $maxy $i r] {
    return "+$rx+$ry"
  }

  #at last attached at (bottom,right)
  if [testCorner rx ry xh yh $xwidth $yheight $maxx $maxy $i r] {
    return "+$rx+$ry"
  }

  # nothing free, return middle top row plus a time depended
  # (pseudo random) value
  return "+400+[expr 2*(1 + [clock seconds] % 10)]"
}


###
### Generate a top level window.
### Disallow new windows during scans.
### Raise window and return if it exists.
### Create a new window and place it to coexist with other
### Xcontrol windows.
###

proc generateToplevel {w title {set ""} {geo ""} {app _}} {
  if [winfo exists $w] {
    raise $w
    return 0
  }

  if {$set != ""} {
    setGlobals 0 $set $app
  }
  global FontSizeIndex
  if {$geo != "" && $FontSizeIndex >= 1} {
    # adjust windows to be higher: 90 % margin to top
    if {4 == [scan $geo "%dx%d+%d+%d" width height gx gy]} {
      set geo ${width}x${height}+${gx}+[expr int(0.9*$gy)]
    } elseif {2 == [scan $geo "+%d+%d" gx gy]} {
      set geo +${gx}+[expr int(0.9*$gy)]
    }
  }
  if [winfo exists $w] {
    # try to set title and icon name for existing widgets
    set wt [winfo toplevel $w]
    wm title $wt $title
    wm iconname $wt $title
    if {$geo != ""} {wm geometry $wt $geo}
    if [winfo ismapped $wt] {
      raise $w
    } else {
      wm deiconify $wt
    }
    return 0
  }

  if {$geo == ""} { set geo [getFree]}
  if {$w != "."} { toplevel $w -class Dialog }
  wm title $w $title
  wm iconname $w $title
  wm geometry $w $geo
  return 1
}


proc giveRoom {w c} {
  # use a scroll frame, if global variable c is set
  if {[info globals $c] == ""} {
    return $w
  }
  global FontSizeIndex
  if {$FontSizeIndex >= 1} {
    set ew 16c;  # edit frame width
    set eh 18c;  # edit frame height
  } else {
    set ew 18c
    set eh 20c
  }

  return [scrollFrame $w right $ew $eh 30c]
}

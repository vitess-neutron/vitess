helpItem {Plotting using Templates} {
VITESS knows three ways to plot 1D (y values at given x positions) and 
2D (z values on a grid of x,y positions) ASCII text data files.

A TclTK plot widgets
B external plot software, especially Gnuplot, integrated to the distribution
C external plot software, called through command shell execution (Unix only)

A is always present, and used bltwish capabilities when bltwish was used.
B has better 1D displays, many users are accustomed to Gnuplot. You may zoom plots
  here, print plots, and use own options in a command window.
  VITESS looks for gnuplot.exe and uses it when present.
C gives full access to external plot software. 

You may fill a template with commands.
A template is a file in the FILES/Plot directory of the installation.
If the first line of a template is a bang line starting with #! under Linux,
this file will be exec'uted as shell script in the background, otherwise it should contain
commands for the specified external plot software (Gnuplot).
  
Some variables like $PFILENAME are substituted before execution of a template.

If you select a given template, you override the default plot.
}

helpItem {Plot template example} {
If you want to plot 2D data (xyz format) with Gnuplot, using a simple red color table, you could
edit the template file gnu2D with those 3 lines
set palette defined (0 "black", 1 "red")
set pm3d map
splot '$PFILENAME'

If you have a perl interface to your plot software, the first 2 lines of a template shell1D could look like
#!/usr/bin/perl
my $fn = '$PFILENAME';

If you like to call gnuplot via sh, you might use
#!/usr/bin/sh
gnuplot -p -e "plot '$PFILENAME'"

or, if the gnuplot process may silently vanish after 10 minutes
#!/usr/bin/sh
gnuplot -e "plot '$PFILENAME'; pause 600"



Those variables are substituted before execution of a template:
$PFILENAME  is the name of the file to be plotted
$PPATH      is the parameter directory 
$PMODULE    name of the pipe module, for autoplots after pipe execution  
$PSKIP      number of lines starting with \# in the beginning of the file
$PROWS      number of rows with data in the file
$PCOLS      number of colums in the file, may be comma separated or free formatted
}

helpItem Gnuplot {
Since version 2.11 VITESS uses Gnuplot, which provides the wxt terminal type to display data
under Windows and Linux in the same fashion. For Mac Os X the terminal type x11 (aqua?) is used.
You may zoom and reposition the plot windows to the size you need, and may zoom inside the data range.

Pressing the P button produces a PDF file of the plot, named <account><unix-time>plot.pdf
in /tmp under Linux and in the parameter directory under Windows.
The p key is bound to printing on the default postscript printer with Linux.

When using TclTK 8.5 error messages from gnuplot are copied to the VITESS output window.

If you need special options to gnuplot, you may use the "Plot Cmd" window, or change an existing
plot template, or just add a file to the FILES/Plot directory for a new plot template.
}

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
  # return either matrix for 2D matrix files, 
  # xyz for 2D files with x y z values
  # xz for files with at least 2 columns of numbers,
  # or "" for insufficient file names/files
  
  if [catch {open $fname r} f] {
    showText "! can't open $fname"
    return ""
  }
  if [eof $f] {
    close $f
    showText "! empty $fname"
    return ""
  }

  while {[gets $f ins] > 0} {
    if {[string range $ins 0 0] != "#"} break
    # check if its a matrix file
    if [regexp {matrix} $ins] {
      close $f
      return matrix
    }
    if [regexp {x  y  z} $ins] {
      close $f
      return xyz
    }
  }
  close $f
  # check if it has more than 16 colums
  eval set ll [list $ins]
  if {[llength $ll] > 16} {
    return matrix
  } elseif {2 > [scan $ins "%f%f%f%f" x y xe ye]} {
    # min. 2 colums of numbers
    showText "! insufficient plot file"
    return ""
  }
  return xz
}

# show 2d array coded with colors
proc show2Dfile {fname} {

  set f [open $fname r]

  set i [getFreePlot]
  set w .plot$i

  set rows 1
  set a {};      # list of row lists, top to bottom
  set xl {};	 # x tic values
  set yl {};     # y tic values

  gets $f ins

  # skip #Monitor line
  if [regexp {^#Monitor} $ins] {
    gets $f ins
  }
  
  set ll [eval list $ins]
  if [string compare "#x y z" "$ll"] {
    set xl $ll;	# first line and first column are tic values
    while {[gets $f ins] > 0} {
      incr rows
      set ll [eval list $ins]
      lappend yl [lindex $ll 0]
      if {$rows == 2} {
        lappend a [set inp [lrange $ll 1 end]]
        set cols [llength $inp]
      } else {
        lappend a [lrange $ll 1 end]
      }
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

  # set color table with 256 entries, taken from gnuplot
  set colist {#000000 #100006 #17000d #1c0013 #200019 #24001f #270026 #2a002c #2d0032 #300038 #32003e #350044 #37004a #3a0050 #3c0056 #3e005c #400062 #420068 #44006d #460073 #470079 #49007e #4b0084 #4d0089 #4e008e #500093 #510098 #53009d #5400a2 #5600a7 #5700ac #5900b0 #5a01b5 #5c01b9 #5d01be #5e01c2 #6001c6 #6101ca #6201cd #6401d1 #6501d5 #6601d8 #6701db #6901de #6a01e1 #6b01e4 #6c01e7 #6d02ea #6f02ec #7002ee #7102f1 #7202f3 #7302f4 #7402f6 #7502f8 #7603f9 #7703fa #7903fb #7a03fc #7b03fd #7c03fe #7d03fe #7e04ff #7f04ff #8004ff #8104ff #8204ff #8305fe #8405fe #8505fd #8605fc #8706fb #8706fa #8806f8 #8906f7 #8a06f5 #8b07f3 #8c07f2 #8d07ef #8e08ed #8f08eb #9008e8 #9108e6 #9109e3 #9209e0 #9309dd #940ada #950ad6 #960ad3 #970bcf #970bcb #980cc8 #990cc4 #9a0cc0 #9b0dbb #9c0db7 #9c0eb3 #9d0eae #9e0ea9 #9f0fa5 #a00fa0 #a0109b #a11096 #a21191 #a3118c #a41286 #a41281 #a5137b #a61376 #a71470 #a7146b #a81565 #a9165f #aa1659 #aa1753 #ab174d #ac1847 #ad1941 #ad193b #ae1a35 #af1b2f #b01b29 #b01c22 #b11d1c #b21d16 #b31e10 #b31f09 #b42003 #b52000 #b52100 #b62200 #b72300 #b72300 #b82400 #b92500 #ba2600 #ba2700 #bb2800 #bc2800 #bc2900 #bd2a00 #be2b00 #be2c00 #bf2d00 #c02e00 #c02f00 #c13000 #c23100 #c23200 #c33300 #c43400 #c43500 #c53600 #c63700 #c63800 #c73900 #c73a00 #c83c00 #c93d00 #c93e00 #ca3f00 #cb4000 #cb4100 #cc4300 #cc4400 #cd4500 #ce4600 #ce4800 #cf4900 #d04a00 #d04c00 #d14d00 #d14e00 #d25000 #d35100 #d35200 #d45400 #d45500 #d55700 #d65800 #d65a00 #d75b00 #d75d00 #d85e00 #d96000 #d96100 #da6300 #da6500 #db6600 #dc6800 #dc6900 #dd6b00 #dd6d00 #de6f00 #de7000 #df7200 #e07400 #e07600 #e17700 #e17900 #e27b00 #e27d00 #e37f00 #e48100 #e48300 #e58400 #e58600 #e68800 #e68a00 #e78c00 #e78e00 #e89000 #e99300 #e99500 #ea9700 #ea9900 #eb9b00 #eb9d00 #ec9f00 #eca200 #eda400 #eda600 #eea800 #eeab00 #efad00 #f0af00 #f0b200 #f1b400 #f1b600 #f2b900 #f2bb00 #f3be00 #f3c000 #f4c300 #f4c500 #f5c800 #f5ca00 #f6cd00 #f6cf00 #f7d200 #f7d500 #f8d700 #f8da00 #f9dd00 #f9df00 #fae200 #fae500 #fbe800 #fbeb00 #fced00 #fcf000 #fdf300 #fdf600 #fef900 #fefc00 #ffff00}

  # plot color bar
  set x1 0
  set x2 [set xdelta [expr $canvaswidth / 256]]
  for {set i 0} {$i < 256} {incr i} {
    #lappend colist [set v [format "#%02x0000" $i]]
    set v [lindex $colist $i]
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

proc getFreeCmdHandle {} {
  global CmdFileInd
  if [info exists CmdFileInd] {
    set CmdFileInd [expr ($CmdFileInd + 1) % 8]
  } else {
    set CmdFileInd 0
  }
  return $CmdFileInd
}

proc closeCmdHandles {} {
  for {set i 0} {$i < 4} {incr i} {
    upvar #0 FH$i gp
    if [info exists gp] {
      catch {close $gp}
    }
  }
}

proc getPlotCmdHandle {app {proceed 1}} {
  global WindowIndex
  upvar #0 FH0 gp
  if [info exists gp] {
    if {$proceed} {
      set WindowIndex [expr ($WindowIndex + 1) % 4]
    }
  } else {
    switch [getSystem] {
      windows {set gp [open "|[list $app] 2>@1" r+]}
      default {set gp [open "|$app 2>@1" r+]}
    }
    set WindowIndex 0
  }
  return $gp
}

proc flushGnuplotCmd {f cmd} {
  if {$cmd != ""} {
    puts $f $cmd
  }
  puts $f "print 'XXXXXX'"
  flush $f
  while 1 {
    if {[gets $f line] <= 0} return
    if [regexp XXXXXX $line] {
      outProtocol "plot done"
      return
    }
    regsub "line 0:" $line "gnuplot:" line
    outProtocol "! $line"
  }
}

proc doGnuplotCmd {w} {
  #  GnuPlotCmd may be set in the template plot window
  global GnuPlotCmd
  set c [string trim $GnuPlotCmd]
  # do nothing, if this entry was blank
  if {$c == ""} return
  set app [getGnuPlotApp]
  if {$app == ""} return
  flushGnuplotCmd [getPlotCmdHandle $app 0] $c
}

proc getGnuplotTerminalType {} {
  global GnuPlotTerminal
  if [info exists GnuPlotTerminal] {
    return $GnuPlotTerminal
  }
  # set the prefered gnuplot type wxt
  set GnuPlotTerminal wxt
  if [catch {set gpt [open "|[getGnuPlotApp] 2>@1" r+]}] return
  puts $gpt "set term wxt"
  puts $gpt "print 'YYY'"
  flush $gpt
  while 1 {
    if {[gets $gpt line] < 0} break
    if [regexp YYY $line] break
    if [regexp unknown $line] {
      # sorry, just plain x11 to be used
      set GnuPlotTerminal x11
      break
    }
  }
  catch {close $gpt}
  return $GnuPlotTerminal
}

proc gnuPlotCmd {app fname ftype} {
  global Plotfile WindowIndex tcl_platform
  set wxt [getGnuplotTerminalType]
  set gp [getPlotCmdHandle $app]
  set wxtcmd "set term $wxt $WindowIndex"
  if {$wxt == "wxt"} {append wxtcmd " size 480,360"}
  puts $gp $wxtcmd

  # keyboard bindings to print and generate PDF files:
  #   keyboard P pressed: generate a PDF file
  set ofn [tmpFilename plot.pdf]
  switch [set mysys [getSystem]] {
    unix {set dummy /dev/null}
    windows {set dummy nul}
  }
  #   second set output command is to close the pdf file
  #   wxt command in the end, to show further plots
  set c "set term pdf color; set o \\\"$ofn\\\"; plot '$fname'; set o \\\"$dummy\\\"; $wxtcmd"
  puts $gp "bind P \"$c\""

  #   keyboard p pressed: send postcript output to the default printer
  #                       for other systems just bind p to generating a PDF file, too
  if {$mysys == "unix"} {
    set c "set term postscript color; set o \\\"|lpr\\\"; plot '$fname'; set o \\\"$dummy\\\"; $wxtcmd"
  }
  puts $gp "bind p \"$c\""
  if {$ftype == "xz"} {
    set com "unset pm3d; plot '$fname'"
  } else {
    set com "set pm3d map; splot '$fname'"
  }
  flushGnuplotCmd $gp $com
}

###
### Templates to plot monitor spectrum data

# Each file in the template directory FILES/Plot may be used as template to plot spectra.
# The plain file names are offered in a selection box for monitor file entries.
# Templates contain plot commands, like
# gnu1D     gnuplot commands to plot a 1D spectrum
# gnu2D                    "           2D spectrum
# If the first line of the file is a bang line, it is a command shell
# suitable for Linux or Mac OS and may use any program suitable to display a spectrum.
# Templates may contain macro variables to be expanded:
# $PPATH $PFILENAME $PMODULE $PSKIP $PROWS $PCOLS

proc saveTemplateFile {w fn} {
  saveTextFile $w $fn "template file"
}

proc getTemplateDir {} {
  global SourceDirectory
  set dir [file join $SourceDirectory FILES Plot]
  if {! [file isdirectory $dir]} {
    if [catch {file mkdir $dir}] {
      showText "unable to create plot template directory $dir"
      return ""
    }
  }
  return $dir
}

proc editTemplate {} {
  if {"" == [set tdir [getTemplateDir]]} return
  set fn [tk_getOpenFile -initialdir $tdir]
  if {$fn == ""} return
  showTextEditWindow .tedit $fn Template 8
}

proc newTemplate {} {
  if {"" == [set tdir [getTemplateDir]]} return
  set fn [tk_getSaveFile -initialfile template1 -initialdir $tdir]
  if {$fn == ""} return
  showTextEditWindow .tedit $fn "Template [file tail $fn]" 8
  if [catch {set lsi [glob -nocomplain -type f [file join $tdir *]]}] return
  global Helpitems
  .tedit.v.text insert end $Helpitems(Plot template example) 
}

proc getPlotTemplates {} {
  set li {}
  if {"" == [set tdir [getTemplateDir]]} return $li
  if [catch {set lsi [lsort [glob -nocomplain -type f [file join $tdir *]]]}] {
    return $li
  }
  foreach fn $lsi {
    if  {[file size $fn] > 0} {
      if {[getSystem] == "windows"} {
        # do not offer shell command files
        if [catch {open $fn r} f] continue
        gets $f line
        close $f
        if {"\#!" == [string range $line 0 1]} continue
      }
      lappend li [file tail $fn]
    }
  }
  if {[llength $li] <= 1} {
    return {}
  }
  return $li
}

proc findFile {roota rootb np {maxlevel 4}} {
  set dirl [list $roota $rootb]
  set ff 0
  for {set i 0} {$i < $maxlevel} {incr i} {
    set lnew {}
    foreach d $dirl {
      set f [file join $d $np]
      catch {
        if [file exists $f] {set ff 1}
      }
      if {$ff} {return $f}
      set pat [file join $d *]
      if [catch {set ssi [glob -nocomplain -type d $pat]}] continue
      foreach dli $ssi {
        lappend lnew $dli
      }
    }
    if {[llength $lnew] <= 0} break
    set dirl $lnew
  }
  return ""
}

proc getGnuPlotApp {} {
  # locate the executable gnuplot program
  global FoundGnuplotApp
  if [info exists FoundGnuplotApp] {return $FoundGnuplotApp}
  switch [getSystem] {
    unix {
	if [catch {exec which gnuplot} res] {set res ""}
	return [set FoundGnuplotApp $res]
    }
    windows {return [set FoundGnuplotApp [findFile C:/ D:/ binary/gnuplot.exe]]}
    default {return [set FoundGnuplotApp ""]}
  }
}

proc getX3DoptfileName {} {
  upvar #0 X3DoptfileName fn
  if [info exists fn] {return $fn}
  if { [getSystem] == "unix"} {
    set fn [globVal env(X3DOPT)]
    if {$fn == ""} {
      set fn [file join [globVal env(HOME)] .x3dopt]
    }
  } else {
    set fn [file join [globVal SourceDirectory] FILES x3d.opt]
  }
  return $fn
}

proc getPreferredX3DCmd {} {
  global PreferredX3DCmd
  set cmd [entryVal x3dapp]
  if {$cmd != "" && [file exists $cmd]} {
    return [set PreferredX3DCmd $cmd]
  }
  if [info exists PreferredX3DCmd] {return $PreferredX3DCmd}
  set ecmd ""
  switch [getSystem] {
    unix {
      set ecmd [globVal env(X3DAPP)]
      if {$ecmd == "" && $cmd != ""} {
	if [catch {exec which $cmd} ecmd] {set ecmd ""}
      }
    }
    windows {
      if {$cmd != ""} {
        if {! [regexp \.(exe|EXE)$ $cmd]} { append cmd .exe }
        set ecmd [findFile C:/ D:/ $cmd]
      }
    }
    default { }
  }
  if {$ecmd != ""} { gSet x3dapp_ $ecmd }
  return [set PreferredX3DCmd $ecmd]
}

proc editX3DOptions {} {
  set fn [getX3DoptfileName]
  if {! [file exists $fn]} {
    if [catch {open $fn w} f] {
      showText "!Could not write x3d option file $fn"
      return
    }
    puts $f {# X3D options
# uncomment and edit lines
# viewport restriction
#xlow=-1
#xhigh=100
#ylow=-1
#yhigh=100
#zlow=-1
#zhigh=100
# material definitions like 
#hullmat=<Material diffuseColor='.3 .3 1' emissiveColor='.1 .1 .33' transparency='.5'/>
# for cubemat rectmat trianglemat cylmat spheremat ellipsmat ellips2mat labelmat
# annotation labels
#fontstyle=<FontStyle DEF='label_font' family='"SANS"' justify='"MIDDLE" "MIDDLE"' size='.1'/>
#labels=0
#transformv2x3d=0
    }
    close $f
  }

  showTextEditWindow .x3dedit $fn "X3D Options" 16 0 100
}

proc getFileDimensions {tfn itemarray} {
  upvar $itemarray la
  if [catch {open $tfn r} f] return
  # count skip lines in the beginning
  set r 0
  set skip 0
  set line ""
  while {[gets $f line] >= 0} {
    if {[string range $line 0 0] != "\#"} {
      set r 1
      break
    }
    incr skip
  }
  set la(PSKIP) $skip
  set la(PSEP) " "
  set c 0
  if {$r} {
    # find number of items
    if [regexp "," $line] {
      set $c [llength [split $line ,]]
      set la(PSEP) ,
    } else {
      foreach i [split $line] {
        if {$i != ""} {incr c}
      }
    }
    set r 1
    while {[gets $f line] >= 0} {
      incr r
    }
  }
  close $f
  set la(PCOLS) $c
  set la(PROWS) $r
}

proc macroExpand {contentvar itemsvar fn} {
  upvar $contentvar content
  upvar $itemsvar items

  # first find items present in content
  foreach item {PATH FILENAME MODULE SKIP ROWS COLS} {
    set s \\\$
    append s P$item
    if [regexp $s $content] {set la(P$item) 1}
  }

  # find replacements for items
  set items [array names la]
  foreach e {SKIP ROWS COLS} {
    if [info exists la(P$e)] {
      getFileDimensions $fn la
      break
    }
  }
  set la(PPATH) [entryVal defdirectory]
  set la(PFILENAME) $fn
  set la(PMODULE) mymodule

  # replace items
  foreach item $items {
    set s \\\$
    append s $item
    regsub -all $s $content $la($item) content
  }
}

proc plotWithTemplate {fn topt} {

  if {"" == [set tdir [getTemplateDir]]} return

  # plot using a template file
  set tfn [file join $tdir $topt]
  if [catch {open $tfn r} f] {
    showText "can't open template file $tfn"
    return
  }
  gets $f line
  if {$line == ""} {
    showText "empty template file $tfn"
    close $f
    return
  }
  if {"\#!" == [string range $line 0 1]} {
    # bang line of a command file
    set bang "$line\n"
    set content ""
  } else {
    set bang ""
    set content "$line\n"
  }
  while {[gets $f line] >= 0} {
    if {$line == ""} continue
    append content "$line\n"
  }
  close $f

  macroExpand content items $fn

  if {$bang != ""} {
    if {[llength $items] > 0} {
      # create a temporary file with macro expanded content
      set tfn [tmpFilename script]
      if [catch {open $tfn w} f] {
        showText "can't write a temporary plot file $tfn"
        return
      }
      puts $f $bang$content
      close $f
    }
    # execute shell script
    catch {exec chmod +x $tfn}
    catch {exec $tfn &}
    if {[llength $items] > 0} {
      # delete the temporary command file, but do not purge it immediately, 
      # because then it may be gone before execution
      after 2000 file delete $tfn
    }
  } else {
    # plot with gnuplot, by piping commands
    set app [getGnuPlotApp]
    if {$app == ""} return
    set gp [getPlotCmdHandle $app]
    if {$gp == ""} return
    foreach s [split $content "\n"] {
      if {$s != ""} {
        #dmf:debug
        #puts "pro gnu :$s:"
        puts $gp $s
      }
    }
    flushGnuplotCmd $gp ""
  }
}

###
### plotMonFile
proc plotMonFile {v app} {
  set fn [entryVal $v $app]
  if {$app != "_tplot_"} {
    set fn [file join [entryVal defdirectory] $fn]
  }
  showPlotFile $fn [entryVal ${v}_o $app]
}

###
### plotFile
proc showPlotFile {name {topt 0}} {

  set ftype [checkPlotfile $name]

  if {$ftype == ""} return
  if {$ftype == "matrix" || $topt == 2} {
    # if requested, or if the file is a 2D monitor file in matrix format,
    # gnuplot may not be used to plot, but we use our own Tcl/Tk code
    show2Dfile $name
    return
  }

  switch $topt {
    "" - "-" - 1 {
      if {"" != [set gcmd [getGnuPlotApp]]} {
        gnuPlotCmd $gcmd $name $ftype
      } else {
        showXYfile $name
      }
    }
    default {
      plotWithTemplate $name $topt
    }
  }
}

proc plotFile {{twod 0}} {
  catch {fileDialog open} name
  if {$name != ""} {showPlotFile $name $twod}
}

proc plotCmdWindow {} {
  set w .x.plotcmd
  generateToplevel $w "Gnuplot Command" "" +20-80
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

proc plotTemplateCmdWindow {} {
  if {[llength [getPlotTemplates]] <= 0} {
    showText "define some plot template first!"
    return
  }
  set w .x.plottcmd
  catch {destroy $w}
  generateToplevel $w "Plot Template Command" "" +20-120
  fileEntry $w.e {fn montemplot "" {"plot file"} "" dat} 8 64 _tplot_
}

proc VisViewer {fn} {
  # visualise neutron trajectories
  global Browser tcl_platform trajmode
  if {$Browser == ""} return
  switch $tcl_platform(platform) {
    unix {
      set url file:$fn
      catch {exec $Browser $url} res
      if [regexp {o running} $res] {
        # try to start the browser with that topic
        catch {exec $Browser $url &}
      }
    }
    default {
      regsub -all / $fn \\ url
      showText "$Browser $url"
      catch {exec $Browser $url &}
    }
  }
  showText "visualise $url"
}
helpItem {Plotting using Templates} {
VITESS knows three ways to plot 1D (y values at given x positions) and 
2D (z values on a grid of x,y positions) ASCII text data files.

A TclTK plot widgets
B external plot software, especially Gnuplot, integrated to the distribution
C external plot software, called through command shell execution

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
If you want to plot 2D data with Gnuplot, you could create a template gnu2d with those 3 lines
set palette defined (0 "black", 1 "red")
set pm3d map
splot '$PFILENAME'

If you have a perl interface to your plot software, the first 2 lines could look like
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

If you need special options to gnuplot, you may use the "Plot Cmd" window, or write
a plot template.
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
  if [catch {open $fname r} f] {
    showText "! can't open $fname"
    return 0
  }
  if [eof $f] {
    close $f
    showText "! empty $fname"
    return 0
  }
  while {[gets $f ins] > 0} {
    if {[string range $ins 0 0] != "#"} break
  }
  close $f
  if {2 > [scan $ins "%f%f%f%f" x y xe ye]} {
    showText "! insufficient plot file"
    return 0
  }
  return 1
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
    if [string match XXXXXX $line] {
      outProtocol "plot done"
      return
    }
    regsub "line 0:" $line "gnuplot:" line
    outProtocol "! $line"
  }
}

proc doGnuplotCmd {w} {
  global GnuPlotCmd
  set c [string trim $GnuPlotCmd]
  if {$c == ""} return
  set app [getPreferredPlotCmd]
  if {$app == ""} return
  flushGnuplotCmd [getPlotCmdHandle $app 0] $c
}

proc useExtPlotCmd {app fname} {
  global Plotfile GnuPlotCmd WindowIndex tcl_platform
  set gp [getPlotCmdHandle $app]
  if {$tcl_platform(os) == "Darwin"} {set wxt x11} else {set wxt wxt}
  set wxtcmd "set term $wxt $WindowIndex size 480,360"
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
  flushGnuplotCmd $gp [set GnuPlotCmd "plot '$fname'"]
}

###
### Templates for plotting data

proc saveTemplateFile {w fn} {
  saveTextFile $w $fn "template file"
  getPlotTemplates
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

proc getPlotTemplates {{withdefault 1}} {
  set li {}
  if {"" == [set tdir [getTemplateDir]]} return $li
  if [catch {set lsi [glob -nocomplain -type f [file join $tdir *]]}] {
    return $li
  }
  if {$withdefault} {lappend li -}
  foreach fn $lsi {
    if  {[file size $fn] > 0} {
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

proc getPreferredPlotCmd {} {
  global PreferredPlotCmd
  if [info exists PreferredPlotCmd] {return $PreferredPlotCmd}
  switch [getSystem] {
    unix {
	if [catch {exec which gnuplot} res] {set res ""}
	return [set PreferredPlotCmd $res]
    }
    windows {return [set PreferredPlotCmd [findFile C:/ D:/ binary/gnuplot.exe]]}
    default {return [set PreferredPlotCmd ""]}
  }
}

proc getFileDimensions {tfn itemarray} {
  upvar $itemarray la
  if [catch {open $tfn r} f] return
  # count skip lines in the beginning
  set skip 0
  set line ""
  while {[gets $f line] >= 0} {
    if {[string range $line 0 0] != "\#"} break
    incr skip
  }
  set la(PSKIP) $skip
  # find number of items
  if [regexp "," $line] {
    set $c [llength [split $line ,]]
    set la(PSEP) ,
  } else {
    set la(PSEP) " "
    set c 0
    foreach i [split $line] {
      if {$i != ""} {incr c}
    }
  }
  set la(PCOLS) $c

  set r 0
  while {[gets $f line] >= 0} {
    incr r
  }
  set la(PROWS) $r
  close $f
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
  if {"\#!" == [string range $line 0 1] && [getSystem] == "unix"} {
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
    # plot using with gnuplot
    set app [getPreferredPlotCmd]
    if {$app == ""} return
    set gp [getPlotCmdHandle $app]
    if {$gp == ""} return
    foreach s [split $content "\n"] {
      if {$s != ""} {
        puts $gp $s
      }
    }
    flushGnuplotCmd $gp ""
  }
}

###
### plotMonFile
proc plotMonFile {type v app} {
  set fn [entryVal $v $app]
  if {$app != "_tplot_"} {
    set fn [file join [entryVal defdirectory] $fn]
  }
  showPlotFile $fn [entryVal ${v}_o $app]
}

###
### plotFile
proc showPlotFile {name {topt 0}} {

  if {! [checkPlotfile $name]} return

  switch $topt {
    "" - "-" - 1 {
      if {"" != [set gcmd [getPreferredPlotCmd]]} {
        useExtPlotCmd $gcmd $name
      } else {
        showXYfile $name
      }
    }
    2 {show2Dfile $name}
    default {plotWithTemplate $name $topt}
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
  global Browser tcl_platform
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
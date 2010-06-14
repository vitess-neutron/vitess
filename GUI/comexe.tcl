### project Vitess
### HMI DN
### M. Fromme fromme@hmi.de
###
### procedures for command execution

proc lookWhosConcerned {serrep serpar serno \
			    modi Comode serll sermol serpal} {
  upvar $serrep srep
  upvar $serpar spar
  upvar $serno sno
  set srep {} ; set spar {} ; set sno {}
  if {$Comode != "ser"} return
  upvar $sermol smol
  upvar $serpal spal
  # handle series: any substitution for this module?
  for {set i 0} {$i < $serll} {incr i} {
    if {[lindex $smol $i] != $modi} continue
    lappend srep \#$i\#
    lappend spar [lindex $spal $i]
    lappend sno $i
  }
}

### compose the VITESS command pipe string
###
proc generateVitessCommand {mode {serll {}} {sermol {}} {serpal {}}} {
  set ll [globVal inputESET]
  set wsh 0
  global Comode Serdefault Plotfile Plottype\
      maxModule DummyEntry SourceDirectory ExeDirectory PipeLogList buffersize

  switch [set Comode $mode] {
    bat - tcl - grd - ser {set prefi \$V}
    default {
      set prefi $ExeDirectory
      set Plotfile {}; set Plottype {}
    }
  }
  upvar #0 ExeSuffix sys
  set logf [tmpFilename vpipelog]
  set PipeLogList {}
  # fc will be the full command
  upvar #0 FullCommand fc
  set fc ""
  lookWhosConcerned srep0 spar0 serno0 0 $Comode $serll sermol serpal
  #  3..6: random seed, random_gen, neutron weight, gravitation effect
  foreach {i} [lrange $ll 3 6] {
    # next proc writes to FullCommand
    writeCommandOption $i _ "" $spar0 $srep0 $serno0
  }

  # add --T option if helper threads are selected, then set name part variable $par, too
  set par ""
  catch {
    if {[entryVal helpthreads] > 0} {
      set par _parallel
      writeCommandOption [lindex $ll 7] _ "" $spar0 $srep0 $serno0
    }
  }

  set pdir [entryVal defdirectory]
  set insert "$fc --B$buffersize --P";	# general command options  
  if {$Comode == "grd"} {append insert \$P} else {append insert $pdir}

  # restart construction of fc, general options have been saved to variable insert
  switch $Comode {
    bat {set fc "\#!/bin/sh\nV=$ExeDirectory\nP=$pdir\n"}
    grd {set fc "\#!/bin/sh\nV=$ExeDirectory\nP=$pdir\nZ=--Z\nL=--L\n"}
    tcl {
      set fc "\#!/usr/bin/tclsh\nset V $ExeDirectory\nset P $pdir\n"
      foreach v {seed gen} vv {SEED TYPE} {
	if {"" == [set t [entryVal random_$v]]} continue
	append fc "set env(GSL_RNG_$vv) $t\n"
      }
      append fc "exec "
    }
    default {set fc ""}
  }
  set first 1

  catch {unset Serdefault};		# will become an array of gui-values for series

  for {set i 1} {$i <= $maxModule} {incr i} {
    set varName mod$i
    upvar #0 $varName var
    if {![info exists var] || $var == $DummyEntry} continue
    set intcom 1
    lookWhosConcerned serrep serpar serno $i $Comode $serll sermol serpal

    switch $var {
      source_const_wave  {set com "source$sys -S1"}
      source_HMI  {set com "source$sys -S1"}
      source_ILL  {set com "source$sys -S1"}
      source_short_pulsed {set com "source$sys -S2"}
      source_ESS {set com "source$sys -S2"}
      source_IPNS {set com "source$sys -S2"}
      source_ISIS {set com "source$sys -S2"}
      source_SNS {set com "source$sys -S2"}
      source_ESS_LPTS {set com "source$sys -S3"}
      chopper_fermi_str {set com "chopper_fermi$par$sys -O1"}
      chopper_fermi_cur {set com "chopper_fermi$par$sys -O2"}
      sm_ensemble {set com "sm_ensemble$par$sys"}
      guide       {set com "guide$par$sys"}
      ma_flat       {set com "monochr_analyser$sys -O1"}
      ma_focus      {set com "monochr_analyser$sys -O2"}
      ma_focus_dat  {set com "monochr_analyser$sys -O3"}
      mon1_lambda {set com "monitor1$sys -k1"}
      mon1_time   {set com "monitor1$sys -k2"}
      mon1_divy   {set com "monitor1$sys -k3"}
      mon1_divz   {set com "monitor1$sys -k4"}
      mon1_y      {set com "monitor1$sys -k5"}
      mon1_z      {set com "monitor1$sys -k6"}
      mon1_energy {set com "monitor1$sys -k7"}
      mon1_divyz  {set com "monitor1$sys -k8"}
      monpol_lambda {set com "monitorpol_1d$sys -k1"}
      monpol_time   {set com "monitorpol_1d$sys -k2"}
      monpol_divy   {set com "monitorpol_1d$sys -k3"}
      monpol_divz   {set com "monitorpol_1d$sys -k4"}
      monpol_y      {set com "monitorpol_1d$sys -k5"}
      monpol_z      {set com "monitorpol_1d$sys -k6"}
      mon2_y_divy  {set com "mon2_posdiv$sys -q1"}
      mon2_z_divz  {set com "mon2_posdiv$sys -q2"}
      external_command {
	set intcom 0
	set com "[globVal extern_com_$i] [globVal extern_shortopt_$i]"
	upvar #0 extern_optfile_$i fname
	forceParamDir fname resfn
	if {$fname != ""} {
	  if {"0" == [catch {open $resfn r} f]} {
	    while {[gets $f ins] > 0} {
	      append com " $ins"
	    }
	    close $f
	  }
	}
      }
      default    {set com $var$sys}
    }
    set logopt $logf$i
    set imore " $insert --L$logopt"
    lappend PipeLogList $logopt
    if $intcom {
      set com [file join $prefi $com]
      if $first {
	append fc "$com$imore"
	#  input file
	writeCommandOption [lindex $ll 0] _ "" $spar0 $srep0 $serno0
	set first 0
      } else {
	append fc " | $com$imore"
      }
      foreach l [globVal ${var}ESET] {
	writeCommandOption $l _$i "" $serpar $serrep $serno
      }
    } elseif {$first} {
      append fc "$com$imore"
      set first 0
    } else {
      append fc " | $com$imore"
    }
  }
  if {$fc != ""} {
    #  output file
    writeCommandOption [lindex $ll 1] _ no_file $spar0 $srep0 $serno0
  }
  if {$Comode == "grd"} {
    # a shell environment will substitute $P
    regsub -all "$pdir/" $fc "\$P/" fc
  }
  # get rid of superfluous blanks
  regsub -all "  " $fc " " fc
  if {$Comode == "bat"} {append fc "\nmv $logf* \$P"}

  return $fc
}

proc checkAll {} {
  clearText
  propagateDigestValues
  set errors [forceParamDir]
  if {$errors} {return 0}
  if [errorWithValues input] {set errors 1}
  global DummyEntry maxModule
  set firstmod ""
  for {set i 1} {$i <= $maxModule} {incr i} {
    set varName mod$i
    upvar #0 $varName var
    if {![info exists var] || $var == $DummyEntry} continue
    if {$firstmod == ""} {
      set firstmod $var
      if {! [regexp {^source_} $var]} {
	if {"" == [entryVal infilename]} {
	  showText "!Please specify an input file, if the first module\ndoes not generate simulated neutrons"
	  set errors 1
	}
      }
    }
    if [errorWithValues $var 1 _$i] {set errors 1}
  }
  if {$errors} {return 0}
  if {$firstmod == ""} {
    showText "!Please specify at least one module."
    return 0
  }

  clearText "description O.K.\n"
  return 1
}

### make copies of simulation results
### to do so we check files in the parameter directory, which are
### newer than the simulation start time

proc copyResults {} {
  global copresults StartTime defdirectory_
  if {$copresults == "no"} return
  set clist {}
  catch {
    foreach fn [glob $defdirectory_/*] {
      if [file isdirectory $fn] continue
      set e [file extension $fn]
      if {$e == ".log"} continue
      file stat $fn s
      set m $s(mtime)
      if {$m <= $StartTime || $s(size) <= 0} continue
      lappend clist $fn
    }
    if {[llength $clist] <= 0} return
    set tdir [file join $defdirectory_ Results]
    set ts [clock format [clock seconds] -format "%Y%m%d-%H%M%S"]
    file mkdir $tdir
    foreach fn $clist {
      file copy $fn [file join $tdir $ts.[file tail $fn]]
    }
  }
}


### check input and generate command output
###
proc checkAction {} {
  if {![checkAll]} return
  # \| in parenthesis !
  # $n for \n, else confusion with n-th match or \n isn't linefeed
  set n "\n "
  regsub -all {\|} [generateVitessCommand check] "$n|" c
  showText "Pipe command would be :\n$c"
}

proc cleanupPipes {} {
  global PipeLogList
  conditionalOpenProtfile
  set errfound 0
  set firstgsl 1
  foreach fname $PipeLogList {
    if {"0" != [catch {open $fname r} f]} continue
    outProtocol "------------------------------"
    while {[gets $f line] >= 0} {
      if [regexp ERROR: $line] {
	outProtocol RRR$line
	set errfound 1
      } elseif [regexp GSL_RNG_ $line] {
	if {$firstgsl} {
	  outProtocol $line
	}
	if [regexp GSL_RNG_SEED $line] {
	  set firstgsl 0
	}
      } else {
	outProtocol $line
      }
    }
    close $f
  }
  copyResults
  if $errfound {
    outProtocol "RRRERRORS OCCURED: Read error messages of the modules concerned"
  }
  conditionalCloseProtfile
  catch {eval file delete $PipeLogList}
}

proc PsCheckUnix {} {
  global PipeIdList
  set rc 0
  if {"0" == [catch {exec ps -p $PipeIdList} res]} {
    set PipeIdList {}
    foreach line [split $res \n] {
      set fi [lindex [split [string trim $line]] 0]
      if [regexp {^[0-9]+$} $fi] {
	# if the first item (blanks omitted) of the ps
	# output line is a number, then it is a process number
	lappend PipeIdList $fi
	set rc 1
      }
    }
  }
  return $rc
}

proc PsCheckWindows {} {
  global PipeIds HelpApp
  set res ""
  if {"0" == [catch {eval exec $HelpApp p $PipeIds} res]} {
    if {$res != ""} {
      set PipeIds $res
      return 1
    }
  } else {
    showText "problems with exec $HelpApp p $PipeIds"
  }
  return 0
}

proc startAction {{sercom ""} {simu simulation}} {
  global PipeActive PipeIds PipeIdsAtStart PipeErr PipeIdList PipeLogList defdirectory_\
      SourceDirectory PsCheck Plotfile Plottype infolevel timeout StartTime
  set c $sercom
  set tool 0
  if {$simu == "tool"} {
    set tool 1
  } elseif {$sercom == ""} {
    if {![checkAll]} return
    set c [generateVitessCommand action]
  } else {
    upvar #0 ExeDirectory V
    upvar #0 defdirectory_ P
  }

  set pname [tmpFilename pipstd.err]
  lappend PipeLogList $pname
  conditionalOpenProtfile

  set n "\n "
  regsub -all {\|} $c "\n|" showcom
  outProtocol "!Parameter directory is $defdirectory_\nStarting $simu\n$showcom"
  set StartTime [clock seconds]

  update
  stopAction 0
  global env
  foreach v {seed gen} vv {SEED TYPE} {
    if {"" == [set t [entryVal random_$v]]} continue
    set env(GSL_RNG_$vv) $t
  }
  if $tool {
    regsub -all \n $c "" c
    set p [pardirPar]
    if {[getSystem] == "windows"} {
      # ein \ für Stringersetzung, ein weiterer für exec !
      # regsub -all / $p {\\\\} p
    }
    append c " --P$p"
    if [catch {eval exec >& $pname $c &} PipeIds] {
      showText "!could not execute tool command\n\t$PipeIds"
      conditionalCloseProtfile
      return
    }
  } elseif [catch {eval exec 2> $pname $c &} PipeIds] {
    showText "!could not start $simu\n\t$PipeIds"
    conditionalCloseProtfile
    return
  }
  set startTime [clock seconds]
  set PipeActive 1
  set PipeIdList [split $PipeIds]
  set PipeIds ""
  foreach p $PipeIdList {
    append PipeIds [format "%x " $p]
  }
  set PipeIdsAtStart $PipeIdList
  outProtocol "$simu ($PipeIds) ($PipeIdList) is active"
  set wsecs 1
  set wmsecs [expr 1000 * $wsecs]
  if {$timeout == "unlimited"} {
    set wsecs 0
  } else {
    set ctout [expr $timeout / $wsecs]
  }
  set i 0
  while {1} {
    if {$wsecs != 0} {
      if {$i == $ctout} {
	outProtocol "!\npipe execution took more than $timeout seconds,\n\tstopping pipe"
	stopAction
      } else {
	showText . ""
      }
      incr i
    } else {
      showText . ""
    }
    if {$PipeActive && [$PsCheck]} {
      after $wmsecs;			# wait for completion,
      update;				# but allow other window events
    } else {
      update
      if {!$PipeActive} {
	showText "doing cleanup"
      }
      cleanupPipes
      set dtime [expr [clock seconds] - $startTime]
      outProtocol "!$simu finished after $dtime s"

      set PipeActive 0
      if {$sercom == ""} {
	set i 0
	foreach p $Plotfile {
	  if {[lindex $Plottype $i] == 1} {
	    showXYfile $p
	  } else {
	    show2Dfile $p
	  }
	  incr i
	}
      }
      conditionalCloseProtfile
      return
    }
  }
}

proc stopAction {{verbose 1} {kill 0}} {
  global PipeActive PipeIds PipeIdList PipeIdsAtStart PipeErr KillProg
  if {[info exists PipeActive] && $PipeActive} {
    if {$kill} {set PipeActive 0}
    switch [getSystem] {
      unix    {
	if {$kill} {
	  catch {eval exec $KillProg -9 $PipeIdList}
	} else {
	  # The sequence of ids in $PipeIdList may have changed.
	  # For correct results we stop processes in pipe order.
	  foreach p $PipeIdsAtStart {
	    if {[lsearch $PipeIdList $p] < 0} continue
	    catch {exec $KillProg $p}
	    after 500;			# wait and let things come to an end
	  }
	}
      }
      windows {
	if {$kill} {set args "k $PipeIds"} else {set args S}
	catch {eval exec $KillProg $args}
	if {$verbose} {outProtocol "!stopping pipe $PipeIdList"}
      }
      default {
	if {$verbose} {showText "!don´t know how to stop processes"}
      }
    }
  }
}

####### Execute / Store Series  ###################

proc dialogSWindow {w {tit "Generate Series"} {where "+100+100"}} {
  catch {destroy $w}
  generateToplevel $w $tit "" $where
  global bgColor
  $w configure  -bg $bgColor
}

proc exeSeries {pdir copy cfiles cdir c ll vl tindl} {
  forceSmallTextWindow
  sizeTextWindow 1 1
  # iconise main vitess window
  wm iconify .x
  global AbortSeries SeriesActive
  set SeriesActive 1
  set AbortSeries 0
  set step 0
  foreach v $vl {
    set com $c
    # substitute special options by list values
    for {set i 0} {$i < $ll} {incr i} {
      regsub \#$i\# $com [lindex $v $i] com
    }
    set pre s[lindex $tindl $step]_
    incr step
    showText "BBB\n\n\nSeries Step $step\nValues: $v\n"
    startAction $com
    if $AbortSeries break
    if {$copy == 0} continue
    catch {
      set so ""
      upvar #0 defdirectory_ P
      foreach fn $cfiles {
	set fb [file join $cdir $pre$fn]
	file copy -force [file join $P $fn] $fb
	append so "\n$fb"
      }
      showText "BBBcopied files:$so"
    }
  }
  wm deiconify .x
  sizeTextWindow
  set SeriesActive 0
}

proc stopSeriesExecution {} {
  global AbortSeries
  set AbortSeries 1
  stopAction 1
}

proc lPack {w t f} {
  global labColor
  label $w -text $t -font $f -bg $labColor
  pack $w -side left
}

proc lPack2 {w t tvar f tw} {
  global labColor entryColor
  label $w.l -text $t -font $f -bg $labColor
  entry $w.e -width $tw -relief sunken -textvariable $tvar -bg $entryColor
  pack $w.l -side left -anchor w
  pack $w.e -side right -anchor e
}

proc storeSeriesFile {w} {
  if {[set f [openWriteFile tcl]] == 0} return
  puts $f [$w.v.text get 1.0 end]
  close $f
  destroy $w
}

proc serSelect {n ser} {
  set ll {0}
  if {$ser == "" || $ser == "all"} {
    for {set i 1} {$i <= $n} {incr i} {
      lappend ll 1
    }
  } elseif {$ser  == "last"} {
    for {set i 1} {$i < $n} {incr i} {
      lappend ll -1
    }
    lappend ll 1
  } else {
    set l [itemize [regsub -all "\[,;\]" $ser " "]]
    for {set i 1} {$i <= $n} {incr i} {
      lappend ll [lsearch $l $i]
    }
  }
  return $ll
}

proc computeMd5 s {
  if [catch {package require md5}] {return ""}
  return [md5::md5 -hex $s]
}

proc isOldMd5 s {
  set fn [file join [entryVal defdirectory] Log $s]
  if [catch {open $fn r} f] {return ""}
  gets $f line
  close $f
  return $line
}

proc storeMd5 s {
  set fdir [file join [entryVal defdirectory] Log]
  if [catch {file mkdir $fdir}] return
  set fn [file join $fdir $s]
  if [catch {open $fn w} f] return
  set s [clock format [clock seconds] -format "%d.%m.%Y %H:%M:%S"]
  puts $f $s
  close $f
}

proc selTypeConv {vv col} {
  global SerRadio SerRadioV

  if [catch {set t $SerRadio($col)}] return
  if {$t == ""} return
  upvar $vv v
  foreach tp [itemize $t ":"] vV [itemize $SerRadioV($col) ":"] {
    if {$v == $tp} {
      set v $vV
      return
    }
  }
}

proc importTable {w} {
  # read a space, tab, or semicolon separated table from a text file
  set rows [entryVal numseries]
  set cols [llength [entryVal modseries]]
  catch {fileDialog open} fname
  if {$fname == ""} return
  if {[catch {open $fname r} f] || [eof $f]} {
    showText "! can't read $fname"
    return
  }
  catch {
    for {set i 1} {$i <= $rows} {incr i} {
      if {[gets $f s] <= 0} break
      set s [itemize $s "; \t"]
      if {[set slen [llength $s]] > $cols} {set slen $cols}
      for {set j 0} {$j < $slen} {incr j} {
	gSet series$i.${j}_ [lindex $s $j]
      }
    }
  }
  close $f
}

proc saveSeries {w {act tofile}} {
  global PipeLogList ExeDirectory Serdefault entryColor labColor bgColor
  set pdir [entryVal defdirectory]
  set cdir [entryVal seriescopytarget]
  set cfiles [itemize [entryVal seriescopyfiles]]
  set num [entryVal numseries]
  set ssel [serSelect $num [entryVal seriesselection]]

  if {$cdir != "" && [llength $cfiles] != 0} {
    catch {file mkdir $cdir}
    if {! [file isdirectory $cdir]} {
      showText "!can't create copy target directory $cdir!"
      return
    }
    set copy 1
  } else {
    set copy 0
  }
  set modl [itemize [entryVal modseries]]
  set ll [llength $modl]
  if {$ll <= 0 || $pdir == ""} {
    showText "!Specify some resonable values or leave it"
    return
  }
  set mol {} ; set pal {} ; set vl {} ; set tindl {}
  foreach m $modl {
    set mil [split $m ":"]
    lappend mol [lindex $mil 0]
    lappend pal [lindex $mil 1]
  }
  set c [generateVitessCommand ser $ll $mol $pal]
  for {set i 1} {$i <= $num} {incr i} {
    if {[set tind [lindex $ssel $i]] >= 0}  {
      lappend tindl $i
      set s {}
      for {set j 0} {$j < $ll} {incr j} {
	set tv [entryVal series$i.$j]
	if {$tv == ""} {catch {set tv $Serdefault($j)}}
	selTypeConv tv $j
	lappend s $tv
      }
      lappend vl $s
    }
  }

  if {$act != "tofile"} {
    # make a md5 hash from the simulation parameters
    set smd5 [computeMd5 "$pdir+$cfiles+$cdir+$c+$ll+$vl"]
    if {[set r [isOldMd5 $smd5]] != ""} {
      if {"no" == [tk_messageBox -icon question -type yesno -title "confirmed command"\
		  -message "This simulation has been done at\n$r\nReally do it again?"]} return
    }
    exeSeries $pdir $copy $cfiles $cdir $c $ll $vl $tindl
    storeMd5 $smd5
    return
  }

  # set tcl commands with variable substitution here:
  set pname [tmpFilename std.err]
  set myexe [info nameofexecutable]
  set fc "#!$myexe
# parameter directory
set P $pdir
# copy these files after each iteration, give \{\} for no files
set CFILES {$cfiles}
# copy them to this directory, give \"\" for no directory
set CDIR \"$cdir\"
# selected indices
set CIND {$tindl}
# list values for parameters
set VL {$vl}
#
# you should know what you're doing if you edit lines below
set LL $ll
set V $ExeDirectory
set COM {$c}
set PipeLogList {$PipeLogList}
set pname $pname
"
  
  foreach v {seed gen} vv {SEED TYPE} {
    if {"" == [set t [entryVal random_$v]]} continue
    append fc "set env(GSL_RNG_$vv) $t\n"
  }

  # no variable substitution here, will be done in script!
  append fc {
lappend PipeLogList $pname

set j -1
foreach v $VL {
  set com $COM
  # substitute special options by list values
  for {set i 0} {$i < $LL} {incr i} {
    regsub \#$i\# $com [lindex $v $i] com
  }
  # execute pipe
  catch {eval exec 2> $pname $com}
  # gather results from temporary pipe log list files
  foreach fname $PipeLogList {
    puts "------------------------------"
    if {"0" != [catch {open $fname r} f]} continue
    while {[gets $f line] >= 0} {puts $line}
    close $f
    }
  catch {eval file delete $PipeLogList}
  # copy files
  if {$CDIR == ""} continue
  set pre s[lindex $CIND [incr j]]_
  foreach fn $CFILES {
    catch {
      set fc [file join $CDIR $pre$fn]
      file copy -force [file join $P $fn] $fc
      puts "copied file: $fc"
    }
  }
}
exit
}

# show tcl text in edit window
  dialogSWindow $w
  frame $w.v -bg $bgColor
  frame $w.b -bg $bgColor
  pack $w.v -side top -fill both -expand yes
  pack $w.b -side top -fill x -expand no
  set lfont [labelFont]
  text $w.v.text -relief raised -bd 2 \
      -height 48 -width 80\
      -font [monoFont] -bg $bgColor\
      -setgrid 1\
      -yscrollcommand "$w.v.yscroll set"
  yscroll $w.v "$w.v.text yview"
  pack $w.v.text -side left -fill both -expand yes -padx 3 -pady 3
  $w.v.text insert end "$fc\n"

  bButton $w.b.b << "inputSeries $w"
  bButton $w.b.n >> "storeSeriesFile $w"
  bButton $w.b.c Cancel "destroy $w"
  pack $w.b.n -side right -anchor w
  pack $w.b.b -side left -anchor w
  pack $w.b.c -side top -anchor w
}

proc setSeriesColumn {j n} {
  if {$n < 2} return
  set delta [entryVal series0.$j]
  if {$delta == ""} return
  if [catch {set delta [expr $delta + 0]}] return
  set v [entryVal series1.$j]
  if {$v == ""} return
  if [catch {set h [expr $v + 0]}] return
  for {set i 2} {$i <= $n} {incr i} {
    gSet series$i.${j}_ [expr $v + ($i - 1) * $delta]
  }
}

proc inputSeries {w} {
  set num [entryVal numseries]
  if {$num == "" || $num <= 0} {
    set e 2
  } else {
    set modl [itemize [entryVal modseries]]
    set ll [llength $modl]
    if {$ll <= 0} {set e 3} else {set e 0}
  }
  if {$e > 0} {
    showText "!Select module:option pairs and give a number of iterations first"
    return
  }
  global entryColor labColor bgColor EntryCharWidth EntryCharHeight

  dialogSWindow $w

  frame $w.t -bg $bgColor
  pack $w.t -side top -fill both -expand yes
  foreach f {p c ct b} {
    frame $w.$f -bg $bgColor
    pack $w.$f -side top -fill x -expand no -padx 3 -pady 3
  }
  set lfont [labelFont]

  set ewid 8
  # find width and height of characters, somehow
  set swid [expr $ewid * $EntryCharWidth * $ll]
  # 2 pixel distance to next entry.
  set shei [expr ($EntryCharHeight + 2)*($num + 3)]
  set ww [scrollFrame $w.t both 8c 6c $shei $swid]

  set mol {} ; set pal {} ; set nal {} ; set nmodl {} ; set vl {}
  foreach m $modl {
    set mil [split $m ":"]
    set m1 [lindex $mil 0]
    lappend mol $m1
    set m2 [lindex $mil 1]
    lappend pal $m2
    lappend nal [lindex $mil 2]
    lappend nmodl $m1:$m2
  }
  for {set i -2} {$i <= $num} {incr i} {
    set g2 ""
    switch -- $i {
      -2 {set g $ww.t2 ; set h Name}
      -1 {set g $ww.t1 ; set h Option}
      0  {set g $ww.t0 ; set h delta; set g2 $ww.hh}
      default {set g $ww.$i ; set h $i.}
    }
    frame $g -bg $bgColor
    pack  $g -side top -fill both -pady 1
    if {$g2 != ""} {
      frame $g2 -bg $bgColor
      pack  $g2 -side top -fill both -pady 4
    }
    lPack $g.h [padString $h 7] $lfont
    for {set j 0} {$j < $ll} {incr j} {
      set m $g.$j
      set a .${j}_
      switch -- $i {
	-2 {
	  set tv seriest2$a
	  gSet $tv [lindex $nal $j]
	  entry $m -width $ewid  -textvariable $tv -state disabled
	}
	-1 {
	  set tv seriest1$a
	  gSet $tv [lindex $nmodl $j]
	  entry $m -width $ewid -textvariable $tv -state disabled
	}
	0 {
	  entry $m -width $ewid -relief sunken -textvariable series0$a -bg $entryColor
	  bind $m <Return> "setSeriesColumn $j $num"
	}
	1  {
	  upvar #0 series1$a vv
	  if [catch {set val $vv}] {
	    # value has not been set before, use value from GUI
	    if {[set n [lindex $nal $j]] != ""} {
	      # entry variables for modules except inputESET have their module number appended
	      if {[set modno [lindex $mol $j]] == 0} {set modno ""}
	      set vv [globVal ${n}_$modno]
	    }
	  }
	  entry $m -width $ewid -relief sunken -textvariable series1$a -bg $entryColor
	}
	default  {
	  entry $m -width $ewid -relief sunken -textvariable series$i$a -bg $entryColor
	}
      }
      pack $m -side left
    }
  }

  set lwid 40
  forceDef seriesselection_ all
  forceDef seriescopytarget_ [file join [entryVal defdirectory] Test]
  lPack2 $w.p "Step\nSelection" seriesselection_ $lfont $lwid
  lPack2 $w.ct "Copy Target\nDirectory" seriescopytarget_ $lfont $lwid
  lPack2 $w.c "Files to\nbe copied" seriescopyfiles_ $lfont $lwid

  bButton $w.b.b << "genSeries $w"
  bButton $w.b.s "Start Series" "saveSeries $w execute"
  bButton $w.b.n "File Series" "saveSeries $w tofile"
  bButton $w.b.i "Import Table" "importTable $w"
  bButton $w.b.c Cancel "destroy $w"
  pack $w.b.s $w.b.n $w.b.i -side right -anchor w
  pack $w.b.b -side left -anchor w
  pack $w.b.c -side top -anchor w
}

proc addSeriesName {modi opt name} {
  global modseries_ moddigest_
  if [info exists modseries_] { append modseries_ " $modi:$opt:$name" }
  if [info exists moddigest_] { append moddigest_ " $name:$modi" }
}

proc genSeries {w} {
  dialogSWindow $w "" "+100+600"

  global entryColor labColor bgColor EntryCharWidth EntryCharHeight
  foreach f {n h1 h2 h3 w b} {
    frame $w.$f -bg $bgColor
    pack $w.$f -side top -fill x -expand no -padx 3 -pady 3
  }

  set lfont [labelFont]

  set ewid 4
  label $w.n.l -text Iterations -font $lfont -bg $labColor -pady 0.5c
  forceDef numseries_ 2
  entry $w.n.e -width $ewid -relief sunken -textvariable numseries_ -bg $entryColor

  # Compute pixel size of entries, to be of use for scrollregion calculations.
  set EntryCharWidth [expr [winfo reqwidth $w.n.e] / $ewid]
  set EntryCharHeight [winfo reqheight $w.n.e]
  pack $w.n.e $w.n.l -side left -anchor w

  label $w.h1.l -text "space separated Module:Option:Name list\nname may be omitted\ne.g. 1:n 3:P"\
      -font $lfont -bg $labColor
  pack $w.h1.l -side left -anchor w
  entry $w.h2.e -width 64 -relief sunken -textvariable modseries_ -bg $entryColor\
      -xscrollcommand "$w.h3.xscroll set"
  pack $w.h2.e -side left -anchor w
  xscroll $w.h3 "$w.h2.e xview"

  bButton $w.b.c Cancel "destroy $w"
  bButton $w.b.n >> "inputSeries $w"
  pack $w.b.c -side left -anchor w
  pack $w.b.n -side right -anchor w
}

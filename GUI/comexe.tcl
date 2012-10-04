### project VITESS
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

proc unzipCom {fname} {
  if {$fname == ""} {return ""}
  if {[file size $fname] < 512}  {return ""}
  if {![regexp {\.([a-z]+)$} $fname r e]} {return ""}
  if {$e == "gz" || $e == "z"} {
    if {[getSystem] == "windows"} {return "[file join [globVal ExeDirectory] gzip.exe] -cd"}
    catch {exec file $fname} res
    if [string match "*gzip compressed data*" $res] {
      # look if zcat is installed
      if [catch {exec which zcat}] {return ""}
      return "zcat"
    }
  }
  return ""
}

# following are global strings whose values are not to be evaluated
set TCL_TOOL {
proc pwrite {fnw pattern} {
  set fo [open $fnw w]
  foreach fn [glob $pattern*] {
    set f [open $fn r]
    while {[gets $f ins] > 0} {puts $fo $ins}
    close $f
    file delete $fn
  }
  close $fo
}
}

# this pwrite is _not_ a tcl, but a perl script
set PERL_TOOL {
sub pwrite {
  my ($fnw, $pattern) = @_;
  open FO,">$fnw";
  foreach $fn (glob("$pattern*")) {
    open F, $fn;
    print FO $_ while <F>;
    close F;
    unlink $fn;
  }
  close FO;
}
}

# do not change indentation in PYTHON_TOOL
set PYTHON_TOOL {
import os
import glob
from string import Template
def pwrite(fn,pattern):
 f=open(fn, 'w')
 for name in glob.glob(pattern+'*'):
  for line in open(name):
   f.write(line)
  os.remove(name)
 f.close
}

### compose the VITESS command pipe string
###
proc generateVitessCommand {mode {serll {}} {sermol {}} {serpal {}}} {

  # mode may be: action bat sh tcl pl py grd ser kstate

  # kstate is used to generate a hash of all settings, and the output should ignore
  # things like the input, output file and overall options, because these are not saved
  # when saving an instrument either.

  set ll [globVal inputESET]
  set wsh 0
  global Comode Serdefault Plotfile Plottype ProgressFile\
      maxModule DummyEntry SourceDirectory ExeDirectory PipeLogList buffersize VisState VisLogList

  switch [set Comode $mode] {
    bat - sh - tcl - pl - py - grd - ser {set prefi \$V}
    default {
      set prefi $ExeDirectory
      set Plotfile {}; set Plottype {}
    }
  }
  upvar #0 ExeSuffix sys
  if {$mode == "kstate"} {
    set logf l
  } else {
    set logf [tmpFilename vpipelog]
  }
  set PipeLogList {}
  set VisLogList {}
  # fc will be the full command
  upvar #0 FullCommand fc
  set fc ""
  lookWhosConcerned srep0 spar0 serno0 0 $mode $serll sermol serpal
  #  3..7: random seed, random_gen, neutron weight, gravitation effect, helper threads
  foreach i [lrange $ll 3 6] {
    # next proc writes to FullCommand
    writeCommandOption $i _ "" $spar0 $srep0 $serno0
  }
  if {$VisState <= 0} {
    # no helper threads with visualisation runs
    writeCommandOption [lindex $ll 7] _ "" $spar0 $srep0 $serno0
  }

  # select parallel image versions for batch processing, ignore this for kstate,
  # and use parallel image version if helper threads have been demanded otherwise
  if {$mode == "kstate"} {set par ""} else {set par _parallel}

  #    default {if {[entryVal helpthreads] > 0} {set par _parallel} }

  set pdir [entryVal defdirectory]
  set insert "$fc --B$buffersize --P";	# general command options
  switch $mode {
    bat - sh - tcl - pl -  py - grd {append insert \$P}
    default {append insert $pdir}
  }

  # restart construction of fc, general options have been saved to variable insert
  switch $mode {
    bat {set fc "V=$ExeDirectory\nP=$pdir\nL=$logf\n"}
    sh  {set fc "\#!/bin/sh\nV=$ExeDirectory\nP=$pdir\nL=$logf\n"}
    grd {set fc "\#!/bin/sh\n\#$ -S /bin/sh\n\#$ -cwd\n\#$ -l vf=1G\nV=$ExeDirectory\nP=$pdir\nL=gridlog\n"}
    tcl {
      set fc "\#!/usr/bin/tclsh[globVal TCL_TOOL]set V $ExeDirectory\nset P $pdir\nset L $logf\n"
      foreach v {seed gen} vv {SEED TYPE} {
	if {"" == [set t [entryVal random_$v]]} continue
	append fc "set env(GSL_RNG_$vv) $t\n"
      }
      append fc "exec "
    }
    pl {
      set fc "\#!/usr/bin/perl[globVal PERL_TOOL]\$V='$ExeDirectory';\n\$P='$pdir';\n\$L='$logf';\n"
      foreach v {seed gen} vv {SEED TYPE} {
	if {"" == [set t [entryVal random_$v]]} continue
	append fc "\$ENV\{'GSL_RNG_$vv'\}='$t';\n"
      }
      append fc "system \""
    }
    default {set fc ""}
  }
  set first 1

  catch {unset Serdefault};		# will become an array of gui-values for series

  set usedIdices {}

  for {set i 1} {$i <= $maxModule} {incr i} {
    set varName mod$i
    upvar #0 $varName var
    if {![info exists var] || $var == $DummyEntry} continue
    set intcom 1
    lookWhosConcerned serrep serpar serno $i $mode $serll sermol serpal

    switch $var {
      chopper_fermi_str {set com "chopper_fermi$par$sys -O1"}
      chopper_fermi_cur {set com "chopper_fermi$par$sys -O2"}
      guide       {set com "guide$par$sys"}
      guide_ideal {set com "guide_elliptic$sys"}
      lense        {set com "lenses$sys"}
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
      mon2_y_divy  {set com "mon2_posdiv$sys -q1"}
      mon2_z_divz  {set com "mon2_posdiv$sys -q2"}
      monpol_lambda {set com "monitorpol_1d$sys -k1"}
      monpol_time   {set com "monitorpol_1d$sys -k2"}
      monpol_divy   {set com "monitorpol_1d$sys -k3"}
      monpol_divz   {set com "monitorpol_1d$sys -k4"}
      monpol_y      {set com "monitorpol_1d$sys -k5"}
      monpol_z      {set com "monitorpol_1d$sys -k6"}
      quadr_field  {set com "sesans_field$sys"}
      sm_ensemble {set com "sm_ensemble$par$sys"}
      source_ESS_2012 {set com "source$sys -S4"}
      source_ESS_LPTS {set com "source$sys -S3"}
      source_HMI  {set com "source$sys -S1"}
      source_FRM2 {set com "source$sys -S1"}
      source_ILL  {set com "source$sys -S1"}
      source_IPNS {set com "source$sys -S2"}
      source_ISIS {set com "source$sys -S2"}
      source_SNS {set com "source$sys -S2"}
      source_const_wave  {set com "source$sys -S1"}
      source_short_pulsed {set com "source$sys -S2"}
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
    switch $mode {
      bat - sh - tcl - pl - py - grd {
        set imore  " $insert --L\$\{L\}$i"
        lappend usedIdices $i
      }
      default {set imore " $insert --L$logopt"}
    }
    switch $VisState {
      1 - 3 {
        lappend VisLogList [set vfile $logf${i}v]
        if {$VisState == 1} {set vopt v} else {set vopt V}
        append imore " --$vopt$vfile"
      }
      default {}
    }
    if {$mode == "action"} {set ppadd  " --p$ProgressFile"} else {set ppadd  ""}
    
    lappend PipeLogList $logopt
    if $intcom {
      set com [file join $prefi $com]
      if $first {
	append com $ppadd
	append fc "$com$imore"
	if {$mode != "kstate"} {
	  # get name of input file
	  set l [lindex $ll 0]
	  set finame [entryVal [lindex $l 0]]
	  if {"" != [set unzip [unzipCom $finame]]} {
	    set fc "$unzip $finame | $fc --c[file size $finame]"
	  } else {
	    writeCommandOption $l _ "" $spar0 $srep0 $serno0
	  }
	}
	set first 0
      } else {
	append fc " | $com$imore"
      }
      foreach l [globVal ${var}ESET] {
	writeCommandOption $l _$i "" $serpar $serrep $serno
      }
    } elseif {$first} {
      append com $ppadd
      append fc "$com$imore"
      set first 0
    } else {
      append fc " | $com$imore"
    }
  }

  if {$fc != "" && $mode != "kstate" } {
    switch [globVal Compmode] {
      case nodebug -
      case nodebug+gzip {set c 1}
      case float -
      case float+gzip {set c 2}
      default {set c 0}
    }
    if $c {
      # get name of output file
      set foname [entryVal [lindex [lindex $ll 1] 0]]
      if {$foname != "" && $foname != "no_file"} {
	append fc " --C$c"
      }
    }
    writeCommandOption [lindex $ll 1] _ no_file $spar0 $srep0 $serno0
  }

  # get rid of superfluous blanks
  regsub -all "  " $fc " " fc

  # for script file output replace parameter directory strings by $P
  switch $mode {
   bat - sh - tcl - pl - py {
     regsub -all "$pdir/" $fc "\$P/" fc
   }
   default {}
  }

  switch $mode {
    bat {append fc "\ntype $logf* > \$P/result.txt\ndel $logf*"}
    sh  {append fc "\ncat $logf? > \$P/result.txt\ncat $logf?? >> \$P/result.txt\nrm $logf*"}
    grd {
      set s ""
      foreach v $usedIdices {
        append s " gridlog$v"
      }
      append fc "\ncat$s > job`date +%s`.log\nrm -f$s\n"
    }
    tcl {append fc "\npwrite \$P/result.txt \$L"}
    pl  {append fc "\";\npwrite(\"\$P/result.txt\", \"\$L\");"}
    py  {
      foreach v {seed gen} vv {SEED TYPE} {
	if {"" == [set t [entryVal random_$v]]} continue
	append oex "GSL_RNG_$vv='$t' "
      }
      set oc "s=Template('$fc')\n"
      append oc "r=s.substitute(V='$ExeDirectory',P='$pdir',L='$logf')\n"
      set fc "\#! /usr/bin/env python"
      append fc [globVal PYTHON_TOOL]
      append fc $oc
      append fc "os.system(\"export $oex;\"+r)\n"
      append fc "pwrite('$pdir/result.txt', '$logf')"
    }
    default {}
  }
  return $fc
}

proc checkAll {} {
  clearText
  propagateDigestValues
  set errors [forceParamDir]
  if {$errors} {return 0}
  global Showinfo
  set Showinfo 0
  if [errorWithValues input] {set errors 1}
  global DummyEntry maxModule VisState
  set firstmod ""
  for {set i 1} {$i <= $maxModule} {incr i} {
    set varName mod$i
    upvar #0 $varName var
    if {![info exists var] || $var == $DummyEntry} continue
    if {$firstmod == ""} {
      set firstmod $var
      if {$VisState > 0} {
        # check for reasonable number_of_neutrons
        set n [entryVal number_of_neutrons _$i]
        if {$n == "" || $n > 100000} {
	  showText "!The number of trajectories for a visualisation run should be at most 100000"
	  set errors 1
        }
      }
      if {! [regexp {^source_} $var]} {
	set infname [entryVal infilename]
	if {"" == $infname} {
	  showText "!Please specify an input file, if the first module\ndoes not generate simulated neutrons"
	  set errors 1
	} elseif {! [file exists $infname]} {
	  showText "!The given input file does not exist"
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

  if {$Showinfo} {
    showText "description O.K.\n"
  } else {
    clearText "description O.K.\n"
  }
  return 1
}

### make copies of simulation results
### to do so we check files in the parameter directory, which are
### newer than the simulation start time

proc copyResults {} {
  global Execmode StartTime defdirectory_
  if {$Execmode != "copy results"} return
  set clist {}
  catch {
    foreach fn [glob -directory $defdirectory_ *] {
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
  if [string match *gzip* [globVal Compmode]] {
    # get name of output file
    set foname [entryVal [lindex [lindex [globVal inputESET] 1] 0]]
    if {$foname != "" && $foname != "no_file"} {
      set foname [file join [entryVal defdirectory] $foname]
      if {[file size $foname] > 512} {
	# compress that file
	outProtocol "try to compress $foname"
	if {[getSystem] == "windows"} {
	  set rc [catch {exec [file join [globVal ExeDirectory] gzip.exe] -f $foname} res]
	} else {
	  set rc [catch {exec gzip -f $foname} res]
	}
	set fzname $foname.gz
	if {$rc == 0 && [file exists $fzname]} {
	  outProtocol "compressed to $fzname"
	} else {
	  outProtocol "did not compress, $res"
	}
      }
    }
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


### save all (flat) files in the parameter directory to a new temporary directory
### within the parameter directory

proc saveEnvironment  {} {
  global FilesBefore TimesBefore Execmode defdirectory_
  if {$Execmode != "save old"} { return "" }
  catch {unset TimesBefore}
  set FilesBefore {}
  set clist {}
  catch {
    foreach fn [glob -directory $defdirectory_ *] {
      set ft [file type $fn]
      if {$ft != "file"} continue
      lappend FilesBefore $fn
      file stat $fn fst
      set TimesBefore($fn) $fst(mtime)
    }
  }
  set tdir ""
  # create a subdirectory of saved files
  for {set i 1} {$i < 1000} {incr i} {
    set tdir [file join  $defdirectory_ "saved_$i"]
    if [file exists $tdir] continue
    file mkdir $tdir
    break
  }
  if {$i >= 1000} {
    # no free slot
    return ""
  }
  foreach fn $FilesBefore {
    file copy $fn $tdir
  }
  return $tdir
}

proc sameMD5Hash {a b} {
  if [catch {package require md5}] {return 0}
  set ahash [md5::md5 -file $a]
  set bhash [md5::md5 -file $b]
  if {"$ahash" == "$bhash"} {return 1}
  return 0
}

### compare contents of envDir with files in the parameter directory
### exchange modified files with old files, and have new files
### in the subdirectory; delete identical copies in the subdirectory;
### delete the subdirectory if empty

proc cleanupEnvDir {{envDir ""}} {
  global FilesBefore TimesBefore defdirectory_
  if {$envDir == ""} return
  if {! [file exists $envDir]} return
  set someremain 0
  set dayname "Xc[clock format [clock seconds] -format "%Y%j"].log"
  catch {
    foreach fn [glob -directory $defdirectory_ *] {
      if [file isdirectory $fn] continue
      if {[lsearch $FilesBefore $fn] != -1} {
	file stat $fn fst
	set tn [file tail $fn]
	set ofn [file join $envDir $tn]
	set remain 0
	if {$fst(mtime) > $TimesBefore($fn)} {
	  # Changed file found, mtime change;
	  # If it is just today's log file: forget about it.
	  if {$tn != $dayname} {
	    # Have contents been changed, too ?
	    # If not, we delete the saved version.
	    if {! [sameMD5Hash $fn $ofn]} {
	      set remain [set someremain 1]
	    }
	  }
	}
	if {$remain == 0} {
	  # file has not been changed, delete the saved version
	  file delete $ofn
	} else {
	  outProtocol "saved old file to $ofn"
	}
      }
    }
  }
  if {$someremain == 0} {
    file delete $envDir
  }
  catch {unset FilesBefore TimesBefore}
}

proc zeroProgress  {} {
  global Progress ProgressS ProgressFile ProgressTimeStart ProgressLT ProcessLastTic
  set Progress [set ProgressS 0]
  set ProcessLastTic 0
  set ProgressTimeStart [clock seconds]
  set ProgressLT ""
  catch {file delete $ProgressFile}
}

proc showProgress {} {
  global Progress ProgressS ProgressFile ProgressTimeStart ProgressLT ProcessLastTic
  set now [clock seconds]
  if [catch {open $ProgressFile r} f] {
    showText . ""
    set Progress [set ProgressS 0]
    return
  }
  set rc [gets $f ins]
  close $f
  if {$rc <= 0 || $ins > 100 || $Progress == $ins} {
    showText . ""
    return
  }

  set Progress $ins
  set ProgressS [expr $ins > 96 ? 97 : $ins]
  if {$Progress <= 0} {
    showText . ""
    return
  }
  set resttime [expr int(($now - $ProgressTimeStart) * (100.0 - $Progress) / $Progress)]
  if {$Progress == 100 || $resttime <= 10} {
    if {"pipe is finishing" == $ProgressLT} {
      showText . ""
    } else {
      showText [set ProgressLT "pipe is finishing"]
    }
    return
  }

  if {[incr ProcessLastTic] < 20} {
    showText . ""
  } else {
    set ProcessLastTic 0
    if {$resttime > 3600} {
      set ProgressLT [format "%02d:%02d hours to finish simulation" [expr int($resttime/3600)] [expr int(($resttime/60)%60)]]
    } elseif {$resttime > 60} {
      set ProgressLT [format "%02d:%02d minutes to finish simulation" [expr int($resttime/60)] [expr int($resttime%60)]]
    } else {
      set ProgressLT "$resttime seconds to finish first module"
    }
    showText $ProgressLT
  }
}

proc reduceFList {ln} {
  upvar #0 $ln glist
  set miss 0
  set rlist {}
  foreach el $glist {
    if [file exists $el] {lappend rlist $el} else {set miss 1}
  }
  if {$miss} {
    set glist $rlist
    return [expr [llength $rlist] <= 0 ? 0 : 1]
  }
  return [expr [llength $glist] <= 0 ? 0 : 1]
}

proc condDelList {ln} {
  upvar #0 $ln glist
  foreach el $glist {
    if [file exists $el] {catch {file delete $el} }
  }
  set glist {}
}

proc doGather {gcom geomfile glist} {
  global defdirectory_
  upvar $glist gl
  if {$gcom == ""} {return ""}
  switch [globVal trajmode] {
    "SVG xz" {set m 1}
    "SVG xy" {set m 2}
    X3D      {set m 3}
    default  {set m 0}
  }

  if {$geomfile != "" && [file exists $geomfile]} {set gex 1} else {set gex 0} 
  switch $m {
    0 {set opt ""
      set ext txt
    }
    1 - 2 {
      if {$m == 1} {set opt " -s"} else {set opt " -z"}
      if {$gex} {append opt " -S $geomfile"}
      set ext svg
    }
    3 { if {$gex} {set opt " -X $geomfile"} else {set opt " -x"}
      set optfilename [getX3DoptfileName]
      if [file exists $optfilename] {append opt " -f $optfilename"}
      set ext x3d
    }
  }

  if {$geomfile == ""} {
    set visRes [tmpFilename _geom.$ext]
  } else {
    # generate a new file name in the parameter directory 
    for {set i 1} {$i < 1000} {incr i} {
      set visRes [file join $defdirectory_ geom_$i.$ext]
      if {! [file exists $visRes]} break
    }
  }

  set com "$gcom$opt -o $visRes $gl"
  # dmf:debug uncommnent next line
  #puts "debug: doing\n$com"
  if [catch {eval exec $com}] {
    # puts "debug: caught exception"
    catch {file delete $visRes}
    return ""
  }
  if [file exists $visRes] {
    return $visRes
  }
  return ""
}
 
proc startActionV {} {
  # start a visualisation run
  global PipeActive VisState VisGather VisMerge VisLogList FilesToDeleteList trajmode
  if {$VisState != 0 || ([info exists PipeActive] && $PipeActive)} {
    showText "!A pipe is still active.\nUse Stop / Kill to finish the running pipe first."
    return
  }
  # puts "debug: startActionV\nVisGather is :$VisGather: VisMerge is :$VisMerge:"
  set firstText ""
  set visRes ""
  if  {$VisGather != ""} {
    # VisState 1 for first --v invocation
    set VisState 1
    startAction "" "" 1
    if [reduceFList VisLogList] {
      # puts "debug: reduceFList VisLogList $VisLogList"
      # gather results of first run
      if {$VisGather == "just-concatenate"} {
        set visRes [tmpFilename _3dvis]
        if {"0" == [catch {open $visRes w} outf]} {
          foreach fname $VisLogList {
            if {"0" != [catch {open $fname r} f]} continue
            while {[gets $f line] >= 0} {
              puts $outf $line
            }
            close $f
          }
          close $outf
        }
      } else {
        set visRes [doGather $VisGather "" VisLogList]
      }
      # result file is to be deleted when VITESS finishes
      lappend FilesToDeleteList $visRes
      set firstText "Find 3D geometry in $visRes"
    }
  }

  condDelList VisLogList

  if {$VisMerge == ""} {
    if {$firstText != ""} {showText $firstText} 
    set VisState 0
    showText "!Computation of trajectories not yet implemented"
    return
  }

  # VisState 3 for --V invocation
  set VisState 3
  startAction "" "" 1

  if [reduceFList VisLogList] {
    # merge visualisation trajectories
    set fullres [doGather $VisMerge $visRes VisLogList]
  } else {
    set fullres ""
  }
  
  # puts "debug: fullres $fullres  trajmode $trajmode"
  # dmf:debug comment next line
  condDelList VisLogList
  set VisState 0

  if {$fullres != ""} {
    if [regexp SVG $trajmode] {
      if {[info procs VisViewer] != ""} {
        # launch SVG viewer = browser
        VisViewer $fullres
      }
    } elseif [regexp X3D $trajmode] {
      set ecom [getPreferredX3DCmd]
      if {$ecom != ""} {
        # launch external X3D viewer
        # dmf:debug uncomment next line
        #puts "doing :$ecom $fullres"
        catch {exec $ecom $fullres &}
      } elseif {[info procs VisViewer] != ""} {
        # launch viewer = browser
        VisViewer $fullres
      }
    }
    showText "Find trajectories in $fullres"
  }
  if {$firstText != ""} {showText $firstText} 
}

proc startAction {{sercom ""} {simu simulation} {visrun 0}} {
  global PipeActive PipeIds PipeIdsAtStart PipeErr PipeIdList PipeLogList defdirectory_\
      SourceDirectory PsCheck Plotfile Plottype Infolevel Checkmode timeout StartTime VisState
  if {($VisState != 0 && $visrun == 0) || ([info exists PipeActive] && $PipeActive)} {
    showText "!A pipe is still active.\nUse Stop / Kill to finish the running pipe first."
    return
  }
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

  set sEnvDir [saveEnvironment]

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
      cleanupEnvDir $sEnvDir
      conditionalCloseProtfile
      return
    }
  } elseif [catch {eval exec 2> $pname $c &} PipeIds] {
    showText "!could not start $simu\n\t$PipeIds"
    cleanupEnvDir $sEnvDir
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
    if {$VisState == 0 || $VisState == 3} {
      if {$wsecs != 0} {
        if {$i == $ctout} {
          outProtocol "!\npipe execution took more than $timeout seconds,\n\tstopping pipe"
          stopAction
        } else {
          showProgress
        }
        incr i
      } else {
        showProgress
      }
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
      if {$sercom == "" && $VisState == 0} {
	foreach p $Plotfile pt $Plottype {
          showPlotFile $p $pt
	}
      }
      cleanupEnvDir $sEnvDir
      conditionalCloseProtfile
      zeroProgress
      return
    }
  }
}

proc stopAction {{verbose 1} {kill 0}} {
  global PipeActive PipeIds PipeIdList PipeIdsAtStart PipeErr KillProg VisState

  zeroProgress

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
  set VisState 0
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
      regsub -all \#$i\# $com [lindex $v $i] com
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
  if {[set f [openWriteFile tcl "" fname]] == 0} return
  puts $f [$w.v.text get 1.0 end]
  close $f
  if {[getSystem] == "unix"}  {
    catch {exec chmod +x $fname}
  }
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
    # now execute the series
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
    regsub -all \#$i\# $com [lindex $v $i] com
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
  set v [entryVal series1.$j]
  set delta [entryVal series0.$j]
  if {$delta == ""} {
    # repeat value
    for {set i 2} {$i <= $n} {incr i} {
      gSet series$i.${j}_ $v
    }
    return
  }
  if {$v == ""} return
  if [catch {set delta [expr $delta + 0]}] return
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

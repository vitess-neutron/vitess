### project Xcontrol
### HMI DN
### M. Fromme fromme@helmholtz-berlin.de
### June 1999

###
### tools to display help text
###

###
### add a help item to the help text array
###
proc helpItem {item itemtext {helparray Helpitems}} {
  upvar #0 $helparray harr
  lappend harr(_items_) $item
  set harr($item) $itemtext
}

proc insertText {w m text} {
  if {[string length $text] > 40} {
    $w insert end "\n$m "
    $w insert end $text
  } else {
    $w insert end "\n$m $text"
  }
}

proc forAllMatches {w pattern script} {
  scan [$w index end] %d numLines
  for {set i 1} {$i < $numLines} {incr i} {
    $w mark set last $i.0
    while {[regexp -indices $pattern \
		[$w get last "last lineend"] indices]} {
      $w mark set first \
	  "last + [lindex $indices 0] chars"
      $w mark set last "last + 1 chars \
	  + [lindex $indices 1] chars"
      uplevel $script
    }
  }
}

proc parseTags w {
  set pp <b\[\^>\]+>;			# show bolds
  forAllMatches $w $pp {
    $w tag add bold first last
    $w delete first "first + 2 chars"
    $w delete "last - 1 chars" last
  }

  $w tag configure bold -font [labelFont]

  set pp <p\[\^>\]+>;			# insert images
  forAllMatches $w $pp {
    set m [$w get "first + 2 chars" "last - 1 chars"]
    $w delete first last
    set fn [file join [globVal SourceDirectory] BITMAPS $m]
    set mp [image create photo -file $fn]
    label $w.$mp -image $mp
    $w window create first -window $w.$mp
  }
}

proc showSelectedHelpItem {w helparray} {
  upvar #0 $helparray harr
  set t $w.t.text
  $t delete 1.0 end
  foreach i [$w.s.s.list curselection] {
    set m [lindex $harr(_items_) $i]
    insertText $t $m $harr($m)
  }
  $t yview 0
  parseTags $t
}


proc showMatchingHelpItems {w helparray {s ""}} {
  upvar #0 $helparray harr
  if {$s == ""} {
    set s [$w.c.e get]
  }
  if {$s == ""} return
  set smatch "*${s}*"
  set t $w.t.text
  $t delete 1.0 end
  foreach m $harr(_items_) {
    set text $harr($m)
    if {[string match $smatch $m] ||
	[string match $smatch $text]} {
      insertText $t $m $text
    }
  }
  $t insert end "\n"
  forAllMatches $t $s {
    $t tag add big first last
  }
  $t tag configure big -background LightSkyBlue2 -relief raised
  parseTags $t
}

proc helpSystem {{w .helpsystem} {helparray Helpitems}} {
  if {![generateToplevel $w "XControl Help"]} {
    return 0
  }
  global tk_version bgColor labColor bgColor Infolevel
  wm minsize $w 30 2
  frame $w.s -bg $bgColor;    pack $w.s -side top -fill both
  frame $w.c -borderwidth 2 -bg $bgColor
  pack $w.c -side top -fill both
  frame $w.t -bg $bgColor
  pack $w.t -side top -fill both -expand yes
  # center scroll list of help items
  set ws $w.s.s
  frame $ws -bg $bgColor;     pack $ws
  if {$tk_version < 4.0} {
    listbox $ws.list -relief raised -borderwidth 2\
	-yscrollcommand "$ws.yscroll set" \
	-geometry 25x6
  } else {
    listbox $ws.list -relief raised -borderwidth 2\
	-yscrollcommand "$ws.yscroll set" \
	-width 25 -height 6
  }
  yscroll $ws "$ws.list yview"
  pack $ws.list
  upvar #0 $helparray harr
  set list $harr(_items_)
  # add descriptions of all variables in *ESET* lists
  foreach s [info globals *ESET*] {
    set m [string range $s 0 [expr [string first ESET $s] - 1]]
    foreach l [globVal $s] {
      if {[lsearch $list [set name [lindex $l 0]]] != -1} continue
      if {[set com [lindex $l 3]] == ""} continue
      lappend list $name
      if {"" == [set desc [lindex $com 1]]} { set desc [lindex $com 0]}
      regsub -all \n "(module $m)\n$desc\ncommand option -[lindex $com end]" \
	  \n\t expl
      set harr($name) $expl
    }
  }
  # sort items
  foreach i [set harr(_items_) [lsort $list]] {
    $ws.list insert end $i
  }
  bind $ws.list <Double-Button-1> "showSelectedHelpItem $w $helparray"

  bButton $w.c.sel "Show selected items"\
      "showSelectedHelpItem $w $helparray"
  label $w.c.l -text "search for ..." -bg $labColor
  myEntry $w.c.e "" 16
  bButton $w.c.don Done "destroy $w"
  if {$Infolevel == "expert"} {
    bButton $w.c.html "Write HTML" helpToHtml
    pack $w.c.sel $w.c.l $w.c.e $w.c.html $w.c.don -side left -expand 1
  } else {
    pack $w.c.sel $w.c.l $w.c.e $w.c.don -side left -expand 1
  }
  bind $w.c.e <Return> "showMatchingHelpItems $w $helparray"

  text $w.t.text -relief raised -bd 2 \
      -height 32 -width 80\
      -font [textFont] -bg $bgColor\
      -setgrid 1\
      -yscrollcommand "$w.t.yscroll set"
  yscroll $w.t "$w.t.text yview"
  pack $w.t.text -side left -fill both -expand yes
  return 1
}

proc showHelpItem {item {w .helpsystem} {helparray Helpitems}} {
  if [regexp {^http:|(html|pdf)$} $item] {
    global SourceDirectory Browser tcl_platform
    if {$Browser == ""} return
    set http [regexp {^http:} $item]
    switch $tcl_platform(platform) {
      unix {
	if $http {
	  set url $item
	  set furl $url
	} else {
	  set url [file join $SourceDirectory WWW $item]
	  set furl file:$url
	}
	catch {exec $Browser $furl} res
	if [regexp {o running} $res] {
	  # try to start the browser with that topic
	  catch {exec $Browser $furl &}
	}
      }
      windows {
	if $http {
	  set url $item
	} else {
	  regsub -all / [file join $SourceDirectory WWW $item] \\ url
	}
	showText "$Browser $url"
	catch {exec $Browser $url &}
      }
    }
    showText "display help info : $url"
    return
  }
  if [helpSystem $w $helparray] {
    showMatchingHelpItems $w $helparray $item
  }
}

proc resolveTags line {
  regsub -all (<b)(\[\^>\]+)> $line {<b>\2</b>} x
  regsub -all (<p)(\[\^>\]+)> $x {<img src="BITMAPS/\2"><p>} nl
  return $nl!
}

proc helpToHtml {{args ""}} {
  if {[set f [openWriteFile "*.htm*"]] == "0"} return
  global Helpitems bgColor
  puts $f {
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>
  <head>
    <title>Virtual Instrumentation (VITESS) for pulsed and continuous sources</title>
  </head>
  }
  puts $f "\n<body bgcolor=\"$bgColor\">"
  puts $f {
    <h1>Virtual Instrumentation (VITESS)<br>for pulsed and continuous sources</h1>
    <a name="Top">
    <ul>
  }
  if {$args == ""} {
    foreach n [array names Helpitems] {
      if [string match \[A-Z\]* $n] {
	lappend args $n
      }
    }
  }
  foreach arg $args {
    puts $f "<li><a href=\"\#$arg\">$arg</a><br></li>"
  }
  puts $f "</ul><hr>"
  foreach arg $args {
    puts $f "<p><H2><A NAME=\"$arg\">$arg</A></H2><pre>"
    foreach l [split $Helpitems($arg) \n] {
      puts $f [resolveTags $l]
    }
    puts $f "</pre>\n"
  }
  puts $f {
    <p><hr><strong>
    <a href="#top">Back to start of page</a><p>
    <a href="https://www.fz-juelich.de/en/jcns/expertise/simulations">VITESS homepage</a><hr>
    <address><a href="mailto:vitess@fz-juelich.de">Email vitess@fz-juelich.de</a></address>
    </strong>
  </body>
</html>
  }
  close $f
  outProtocol "stored html file"

}

helpItem {Getting Started} {
You can either start from scratch or with one of our examples, collected in the FILES sub-folder
of the VITESS installation directory (menu 'File' | 'LOAD Instrument').
In the first case, we recommend to create a new directory first and to define it
as 'parameter directory'.
'input file' and 'output file' are only needed in special cases.
The second step is to create a source.
Click on the '--inactive--' button and choose your kind of source. You will get a default
'moderator description file'; this contains most of the information of the moderator.
For your own simulation you will probably either have to choose another description
(Browse in Files/moderators) - or to change the contents of the default file (Edit).
}

helpItem {Inserting/Deleting a Module} {
To insert a new module just select a real module from the list of all modules,
a popup menu shows up when clicking on the --inactive-- button.

If you click on the module number of a real module in the list, a popup menu offers to
- Move Down : move rest of the instrument down to allow insertion of a module above
- Remove Module : delete this module from the list
- Edit here : edit the module parameters in the frame to the right of the module list
  (same action clicking right arrow (Windows+Linux) or module number (Mac) 
- Separate Window : edit the module parameters in a separate window
- Disable Module : you may deactivate a module, leave it out from execution
- re-enable this or all disabled modules
- generate a X3D visualisation of that module alone

Modules may temporarily disabled/deactivated. Those module are shown with white module
numbers in the module list, and are skipped when executing the simulation.
If you change the module list (insert or edit or delete a module) all modules
are activated again.
}

helpItem {Visualising Results} {
To visualise the result of your simulation, you should use a monitor, e.g. 'mon1_lambda'
to see the wavelength dependence of the intensity.
Click on the '--inactive--' button and choose 'monitor' -> 'mon1_lambda'.
}

helpItem {Saving an Instrument} {
You may store settings of your assembled simulation to an instrument file
with .gui extension (menu 'File' | 'save instrument').
All parameter settings are part of the resulting file, so that you or others
may load that instrument on a later simulation, if you provide that file along with other
files refered from the parameter directory.
(There are few exceptions: Options and some global variables which would prevent
the execution under a different environment are not saved here.)
}

helpItem {Recover an Instrument} {
The GUI stores the instrument silently to snapshots when you change parameters.
You may recover a stored snapshot via menu Edit / Recover Instrument:
select a snapshot of your session some time ago.
The default is to take a snapshot per minute, if something has been changed.
There are up to 8 latest snapshots available.
You may change these numbers via menu Options / Recovery.
Snapshots will be deleted on VITESS exit.
}

helpItem {Packages} {
Packages are stripped down parts of an instrument, like cascaded guides or monitors.
You may save settings of consecutive modules as a package (menu 'File' | 'SAVE Package').
A package may be added after the last module (menu 'File' | 'ADD Package'), or inserted
after a module given by number (menu 'File' | 'INSERT Package').
}

helpItem Troubleshooting {
Input errors are partly checked by the GUI - click 'Check' to do this.
The GUI tells if it found errors in the control window (bottom of screen).
A more thorough check may be done with 'Dryrun'. A dryrun is a pipe execution with
few neutron trajectories, where temporary result files become deleted.

In order to find possible problems, module messages should be read in the control window.
1) "could not start pipe couldn't execute "C:\Program": no such file or directory"
   Reason: The path contains a 'blank'. (File names and paths must not have a 'blank'.)
2) "could not start simulation
    couldn't write file "C:/temp/pipstd.err" permission denied"
   Reason: Directory C:\temp does not exist (and cannot be created) or is not writeable.
3) "ERROR: Can't open xyz.dat to read user given ... distribution"
   Reason: A wrong path to the input file has been given.
}

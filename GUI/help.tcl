### project Xcontrol
### HMI DN
### M. Fromme fromme@hmi.de
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
  global tk_version bgColor labColor bgColor infolevel
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
  if {$infolevel == "expert"} {
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
  if [regexp {(html|pdf)$} $item] {
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
	catch {exec $Browser -remote openURL($furl)} res
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
    <a href="http://www.hmi.de/projects/ess/vitess/index.html">VITESS homepage</a><hr>
    <address><a href="mailto:vitess@hmi.de">Email vitess@hmi.de</a></address>
    </strong>
  </body>
</html>
  }
  close $f
  outProtocol "stored html file"

}

helpItem {Getting Started} {
You can either start from scratch or with one of our examples, collected in the FILES sub-folder
of the Vitess installation directory (menu 'File' | 'load instrument').
In the first case, we recommend to create a new directory first and to define it as 'parameter directory'.
'input file' and 'output file' are only needed in special cases.
The second step is to create a source.
Click on the '--inactive--' button and choose your kind of source. You will get a default
'moderator description file'; this contains most of the information of the moderator.
For your own simulation you will probably either have to choose another description
(Browse in Files/moderators) - or to change the contents of the default file (Edit).
}

helpItem {Inserting/Deleting a Module} {
The 'X' button on the left side of the big 'module'-button deletes this module.
The 'arrow-down' button moves the rest of the instrument down to allow insertion of a module.
The 'arrow-up' button shows the module in a separate window.
The 'arrow-right' button shows the module parameters here, replacing this introduction.
}

helpItem {Visualsing Results} {
To visualise the result of your simulation, you should use a monitor, e.g. 'mon1_lambda'
to see the wavelength dependence of the intensity.
Click on the '--inactive--' button and choose 'visualise_data' -> 'mon1_lambda'.
}

helpItem Troubleshooting {
Input errors are partly checked by the GUI - click 'Check' to do this.
If the command line occurs in the control window (bottom of screen), the GUI has found no mistakes.
In order to find possible problems, error messages should be read in the control window.
If the pipe command cannot be executed, 
1) "could not start pipe couldn't execute "C:\Program": no such file or directory"
   Reason: The path contains a 'blank'. (File names and paths must not have a 'blank'.)
2) "could not start simulation
    couldn't write file "C:/temp/pipstd.err" permission denied"
   Reason: Directory C:\temp does not exist (and cannot be created) or is not writeable.
3) "ERROR: Can't open xyz.dat to read user given ... distribution"
   Reason: A wrong path to the input file has been given.
}

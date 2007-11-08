#!/usr/bin/perl

# Script to patch a Tcl/Tk unix installation of a known tkfbox.tcl bug
# M.Fromme HMI Nov 2007

use strict;

sub usage {
  print <<EOS;
usage:
patchTkUnix [-w wish] [-p patchfile]
  -v                verbose
  -w wish           absolute wish path, default is to search bltwish
  -p patchfile      alternate name for replacement file
generates a replacement file for tkfbox.tcl
EOS
  exit 0;
}

my ($wish, $isLinux, $anyErr, $ldd, $verbose, $patched);
my $patchfile = 'patched_tkfbox.tcl';

while ($_ = shift) {
  if (/^-(.)/) {
    if ($1 eq 'v') {
      $verbose = 1;
    } elsif ($1 eq 'w') {
      $wish = shift;
    } elsif ($1 eq 'p') {
      $patchfile = shift;
    } else {
      usage();
    }
  } else {
    usage();
  }
}

my $sysapp = mySystem();

if ($wish eq '') {
  $wish = findImage('bltwish');
  $wish = findImage('wish') unless -x $wish;
}

myexit ('Cannot find a Tcl/Tk wish executable. Please install Tcl/Tk first!')
  unless $wish ne '' && -x $wish;

#check version of wish
$_ = `echo 'puts \$tk_version; exit' | $wish 2>&1`;
myexit("Please set a valid display environment variable to test and install!")
   if /couldn't connect to display/;
chomp;
if ($_ ne '' && $_ < 8.2) {
  print "Version $_ of $wish too old (elder than 8.2)!\n";
  usage();
}

my $ofile = $_;

# search for tk library and check version

$_ = `echo 'puts \$tk_library; exit' | $wish 2>&1`;
myexit("set a valid display environment variable to test tkfbox.tcl!")
  if /couldn't connect to display/;
chomp;
my $tkfbox = "$_/tkfbox.tcl";
$anyErr .= "can't read and test $tkfbox\n" unless -s $tkfbox;
my $version;
open F, $tkfbox;
if ($patchfile) {
  $patchfile = '' unless open OF, ">$patchfile";
}
while (<F>) {
  if (/^([\s]+)(\$data\(typeMenuLab\) config -state normal)/) {
    $patched = 1;
    print OF "$1catch \{ $2 \}\n" if $patchfile;
  } else {
    $version = $1 if /\$Id:[\s]+tkfbox\.tcl.+v[\s]+([0-9.]+)/;
    print OF $_ if $patchfile;
  }
}
close F;
if ($version ne '') {
  print "$tkfbox\n\t found version $version\n" if $verbose || $patchfile;
} else {
  print "can't extract version of $tkfbox\n";
}
close OF;
if ($patched) {
  $_ = "$patchfile $tkfbox";
  print "Replacement patch file written to $patchfile\ndiff $_\n";
  system "diff $_";
  print "\nroot, please do\ncp $_\n";
} else {
  print "$tkfbox already patched\n";
  unlink $patchfile;
}

sub myexit {
  print "Error: $_[0]\n";
  exit(2);
}

sub findImage {
  my $p = $_[0];
  foreach (split ':',$ENV{PATH}) {
    foreach (<$_/$p>) {
      return $_ if -x $_;
    }
  }
  '';
}

sub mySystem {
  $_ = `uname`;
  chomp;
  return "_$_" if $_ ne 'Linux';
  $isLinux = 1;
  $_ = `uname -i`;
  return '_Linux_x86_64' if /x86_64/;
  '_Linux';
}

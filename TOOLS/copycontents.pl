#!/usr/bin/perl

use File::Copy;
use strict;

my ($d, $subn, @Files, @Sub);

my $sourcedir = shift;
my $targetdir = shift;

$sourcedir =~ s/\/+$//;
$targetdir =~ s/\/+$//;

die("give source directory") unless -d $sourcedir;
die("give target directory") if $targetdir eq '';

mkdir $targetdir, 0755 unless -d $targetdir;

die("could not create target directory $targetdir;") unless -d $targetdir;

my @D = ($sourcedir);
while ($d = shift @D) {
  opendir R, $d;
  while ($_ = readdir(R)) {
    next if /^\./;        # no point directories alltogether
    next if /(log|~)$/;   # no log files, no old edit files
    # print "$_\n" unless /^[a-zA-Z0-9_.+-]+$/;
    next unless /^[a-zA-Z0-9_.+-]+$/;
    my $fn = "$d/$_";
    next if -l $fn;       # no symbolic links
    $fn =~ m@^$sourcedir/(.+)$@;
    $subn = $1;
    if (-d $fn) {
      push @D, $fn;
      push @Sub, $subn;
    } else {
      push @Files, $subn;
    }
  }
  closedir R;
}

foreach $d (@Sub) {
  mkdir "$targetdir/$d", 0755;
  # print "mkdir 0700, $targetdir/$d\n";
}

foreach (@Files) {
  copy("$sourcedir/$_", "$targetdir/$_");
  # print "$sourcedir/$_ $targetdir/$_\n";
}

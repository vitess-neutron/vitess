#!/usr/bin/perl

use File::Copy;
use strict;

my ($d, $subn, $testonly, $sourcedir, $targetdir, $verbose, @Files, @Sub);

while ($_ = shift) {
    if (/^-t/) {
	$testonly = 1;
    } elsif (/^-v/) {
	$verbose = 1;
    } elsif ($sourcedir) {
	$targetdir = $_;
    } else {
        $sourcedir = $_;
    }
}

$sourcedir =~ s/\/+$//;
$targetdir =~ s/\/+$//;

die("give source directory") unless -d $sourcedir;
die("give target directory") if $targetdir eq '';

unless ($testonly) {
    mkdir $targetdir, 0755 unless -d $targetdir;
    die("could not create target directory $targetdir;") unless -d $targetdir;
}

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
    $_ = "$targetdir/$d";
    next if -d $_;
    if ($testonly) {
	print "would mkdir $_\n";
    } else {
	print "mkdir $_\n" if $verbose;
	mkdir $_, 0755;
    }
}

foreach (@Files) {
  mycopy("$sourcedir/$_", "$targetdir/$_");
}

sub mycopy {
    my ($a, $b) = @_;
    return unless -s $a;
    my $doit;
    if (-s $b) {
	# do not overwrite, if same contents
	my @A = stat $a;
	my @B = stat $b;
	if ($A[7] != $B[7]) {
	    $doit = 1;
	} else {
	    $doit = `diff -q $a $b` ? 1 : 0;
	}
    } else {
	$doit = 1;
    }
    return unless $doit;
    if ($testonly) {
	print "would copy $a, $b\n";
    } else {
	print "copy $a $b\n" if $verbose;
	copy($a, $b);
    }
}

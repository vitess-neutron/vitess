#!/usr/bin/perl

my $root = $ARGV[0];
$root = $ENV{PWD} if $root eq '';
my $bindir = $root . 'VitessModulesUnix';
my $tdir   = $root . 'SRC/vitess/trunk/MODULES';

opendir R, $bindir;
while ($_ = readdir(R)) {
  next if -d $_;
  symlink "$bindir/$_", "$tdir/$_";
}
closedir R;

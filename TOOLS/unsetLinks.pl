#!/usr/bin/perl

my $root = $ARGV[0];
$root = $ENV{PWD} if $root eq '';
my $tdir = $root . 'SRC/vitess/trunk/MODULES';

opendir R, $tdir;
while ($_ = readdir(R)) {
  $_ = "$tdir/$_";
  unlink $_ if -l $_;
}
closedir R;

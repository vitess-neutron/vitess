#!/usr/bin/perl

my $root = $ARGV[0];
$root = $ENV{PWD} if $root eq '';
my $bindir = $root . 'VitessModulesUnix';
my $tdir   = $root . 'SRC/vitess/trunk/MODULES';

opendir R, $tdir;
while ($_ = readdir(R)) {
  next if -d $_;
  next unless /_(Linux|Linux_x86_64|Darwin|Darwin_x86_64|OSF1|SunOS)$/;
  next if /^rvitess_/;
  my $binary = "$tdir/$_";
  next if -l $binary;
  rename $binary, "$bindir/$_";
}
closedir R;

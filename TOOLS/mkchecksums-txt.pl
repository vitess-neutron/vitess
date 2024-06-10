#!/usr/bin/perl

use strict;

# erzeuge checksums<id>.txt zu vitess<id>.(exe|tar.gz|7z)

my $usage = <<EOS;
usage:
$0 \[id\]
      erzeugt checksums.txt für VITESS Pakete
id    Versionsangabe, z.B. 3.4
EOS

my ($id, $ext, $major, $minor, @CRC, @MD5, @SHA1);

$_ = shift;
if ($_ eq '') {
  opendir D, '.';
  while (($_ = readdir D)) {
    next unless /^vitess([1-9]+)\.([0-9]+)([a-d])\.exe$/;
    if ($major eq '' || $major < $1 ||
        ($major eq $1 && $minor < $2) ) {
      $major = $1;
      $minor = $2;
    }
  }
  closedir D;
  die $usage unless $major;
  $id = "${major}_$minor";
} else {
  die $usage unless /^([1-9]+)\.([0-9]+)[a-d]?$/;
  $id = $_;
}
my $v = "vitess$id";
die $usage unless -x "$v.exe";

$_ = `cd /hmi/dmf/SRC/vitess/trunk; svn info -r HEAD`;
/\nRevision: ([0-9]+)/;
print "VITESS $id\nSubversion Revision $1.\n\nWeblinks\n\n";

foreach $ext (qw(exe tar.gz 7z)) {
  my $vn = "$v.$ext";
  $_ = `cksum $vn`;
  chomp;
  push @CRC, $_;
  $_ = `md5sum $vn`;
  chomp;
  push @MD5, $_;
  $_ = `sha1sum $vn`;
  chomp;
  push @SHA1, $_;
}

print "\ncksum (CRC32)\n", join("\n", @CRC), "\n";
print "\nmd5sum (MD5 message digest)\n", join("\n", @MD5), "\n";
print "\nsha1sum\n", join("\n", @SHA1), "\n";

$_ = localtime();
print "\n$_\n";

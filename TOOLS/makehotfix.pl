#!/usr/bin/perl

# Extract source files of VITESS to an archive file.

# This tool is meant for a VITESS developer using Linux.
# The directory $svndir must be a working directory, checked out
# from the repository $svnrepository.
# Files from the repository are then extracted with the credentials of the
# developer calling this script.

use File::Copy;
use strict;

###
# configure here
my $svnrepository = 'http://www.helmholtz-berlin.de/svn/vitess/trunk';
my $svndir = '/hmi/dmf/SRC/vitess/trunk';
# end configure
###

sub usage {
  print <<EOS;
usage:
hotfix.pl [option]  [archive_name]
option may be
-l              list files only, in conjunction with -v subnumber
-r number       consider files changed since subversion revision number
                if no archive_name is given, files are listed only
                if no number is given, and an archive_name, the whole repository
                will be extracted
-v              be verbose

archive_name, if given, should be an archive file name with an extension
from the list tar,tgz,bz2,7z, or zip.
If you specify eg. dummy.zip, the name of the file will be generated to reflect
the contents, like arc443.zip for an archive of the full svn export of revision 433,
and arc430fix443.zip for an hotfix archive with new files since revision 430 to
the head revision 443.
EOS
  exit 0;
}

my ($ofn, $revision, $headrev, $tdir, $verbose, $ext, @F);

while ($_ = shift) {
  if (/^-(.*)/) {
    $_ = $1;
    if (/r/) {
      $_ = shift;
      &usage unless /^([0-9]+)$/;
      $revision = $1;
    } elsif (/v/) {
      $verbose = 1;
    } else {
      &usage;
    }
  } else {
    $ofn = $_;
  }
}

if ($ofn eq '' && $revision ne '') {

  &fetchFileList;
  print "$_\n" foreach @F;

} elsif ($ofn =~ /^(.+)\.(tar|tgz|7z|zip|bz2)$/) {
  $ext = $2;
  if ($1 eq 'dummy') {
    # get head revision number
    $_ = `cd $svndir ; svn info`;
    if (/Revision: ([0-9]+)/g) {
      $headrev = $1;
    } else {
      $headrev = 'head';
    }
    if ($revision ne '') {
      $ofn = "arc${revision}fix$headrev.$ext";
    } else {
      $ofn = "arc$headrev.$ext";
    }
  }
  my $now = time;
  $tdir = "/tmp/${now}hot$$";
  mkdir $tdir, 0765;
  die "can not create temp dir" unless -d $tdir;
  if ($revision ne '') {
    &fetchFileList;
    &copyHotfixFiles;
  } else {
    &extractFull;
  }
  &createArchive;
  print "created archive $ofn\n" if $verbose || $headrev ne '';

  if (-d $tdir && $tdir =~ m@^/tmp/@) {
    system "rm -r $tdir";
  }
  #print "look at $tdir\n";

} else {
  &usage;
}


sub fetchFileList {
  open F, "cd $svndir; svn diff --summarize -r$revision:HEAD|";
  while (<F>) {
    next unless /^[AM]\s+(.+)$/;
    push @F, $1;
  }
  close F;
}

sub copyHotfixFiles {
  my ($ffn, $fn, $d);
  foreach (@F) {
    $ffn = $_;
    if (m@/@) {
      m@^(.+)/([^/]+)$@;
      $d = $1;
      $fn = $2;
      #print "makeSubdirs($tdir, $d)\n";
      makeSubdirs($tdir, $d);
      copy("$svndir/$ffn", "$tdir/$ffn");
    } else {
      copy("$svndir/$ffn", "$tdir/$ffn");
    }
  }
}

sub extractFull {
  if ($verbose) {
    system "cd $tdir ; svn export $svnrepository vitess";
  } else {
    system "cd $tdir ; svn export $svnrepository vitess &>/dev/null";
  }
}

sub createArchive {
  my $v = $verbose ? 'v' : '';
  if ($ext eq 'tgz') {
    $_ = "tar czf$v";
  } elsif ($ext eq 'tar') {
    $_ = "tar cf$v";
  } elsif ($ext eq '7z') {
    $_ = '7za a';
  } elsif ($ext eq 'bz2') {
    $_ = "tar cIf$v";
  } elsif ($ext eq 'zip') {
    $_ = "zip -rq$v";
  }
  #print "com :$_:  v :$v:\n";
  # we need an absolute path for ofn
  $ofn = "$ENV{PWD}/$ofn" unless $ofn =~ m@^/@;
  my $com = "cd $tdir ; $_ $ofn *";
  #print "$com\n";
  if ($verbose) {
    system $com;
  } else {
    system $com . ' &>/dev/null';
  }
}

sub makeSubdirs {
  my ($dir, $d) = @_;
  foreach (split '/', $d) {
    $dir .= "/$_";
    mkdir $dir, 0765 unless -d $dir;
  }
}

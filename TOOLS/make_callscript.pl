#!/usr/bin/perl

# makes vitess call script for an unix multi user installation

use File::Spec;
use strict;

my ($install_dir, $full_copy, $script_filename, $wish, $isLinux);

$_ = File::Spec->rel2abs($0);
$install_dir = $1 if m@^(/.+)/TOOLS/@;

my $usage =<<EOS;
usage:
$0 [option] script_filename

To be called from an existing vitess root installation, which will be used
by individual users.
(Assumed root installed /opt/vitess3.4 users may not write files there, but
should use an own vitess directory like ~/vitess3.4. This script, when
used the first time from users, copies some files from the root installation,
and makes the bulk available by symbolic links.)

script_filenme should be in the user PATH like /usr/local/bin/vitess3.4 .

option may by
-c   make a full copy of installed files to user directories
     default is to use symbolic links for BITMAPS Concepts GUI MODULES OPTIMIZATION
     RelNotes SRC TOOLS WWW .
-i install_dir   directory where root installed vitess, default $install_dir
EOS

while ($_ = shift) {
  if ($_ eq '-c') {
    $full_copy = 1;
  } elsif (/^-(.*)/) {
    $_ = $1;
    if (/i/) {
      $install_dir = shift;
    } else {
      myexit($usage);
    }
  } else {
    $script_filename = $_;
  }
}

myexit("Please specify the vitess installation directory!\n") unless -d $install_dir;
myexit("Please provide a script_filename!\n") unless $script_filename;

# look for wish
$wish = findImage('wish');

$_ = checkBinaries();
myexit($_) if $_;

# look for vitess version
$_ = `grep 'set t "VITESS ' $install_dir/GUI/control.tcl`;
/VITESS ([0-9.]+)/;
my $version = $1 ne '' ? $1 : 'ownversion';

# create vitess call script
open O, ">$script_filename";

print O <<'EOS';
#!/usr/bin/perl

# wrapper script to be called from users to run vitess,
# using local copies of installation files.

use strict;

EOS

$_ = $full_copy ? ' = 1' : '';
print O <<EOS;
my \$install_dir = '$install_dir';
my \$local_subdir = 'vitess$version';
my \$wish = '$wish';
my \$full_copy$_;
EOS

print O <<'EOS';

my $local_dir = "$ENV{HOME}/$local_subdir";
my $hidden_file = "$local_dir/.ok";

if (-d $local_dir) {
  # check if it has been created from us
  die "had $local_dir been installed directly?\n" unless -f $hidden_file;
} else {
  &makeLocalCopy;
}

my $tcl_source = "$local_dir/Vitess";
die "cannot execute $tcl_source\n" unless -s $tcl_source;

exec $wish, $tcl_source;

sub makeLocalCopy {

  mkdir $local_dir, 0700;
  die "cannot create $local_dir" unless -d $local_dir;

  # touch hidden file
  open O, ">$hidden_file";
  close O;

  foreach (qw(BITMAPS Concepts GUI MODULES OPTIMIZATION
              RelNotes SRC TOOLS WWW gridrun license.txt)) {
    if ($full_copy) {
      system "cp -pr $install_dir/$_ $local_dir/";
    } else {
      symlink "$install_dir/$_", "$local_dir/$_";
    }
  }
  system "cp -pr $install_dir/FILES $local_dir/";

  # Extract Vitess tcl start script
  my $local_script = "$local_dir/Vitess";
  open O, ">$local_script";
  open F, "$install_dir/Vitess";
  while (<F>) {
    last if /^proc VScroll/;
  }
  print O "### VITESS simulation GUI start script\nset defdirectory_ $local_dir\nset NoBLT 1\n\n$_";
  print O $_ while <F>;
  close F;
  close O;
  print "copied local vitess files to $local_dir\n";
}
EOS

close O;
die "could not write $script_filename!\n" unless -s $script_filename;

chmod 0755, $script_filename;
print "created user call script $script_filename for vitess version $version .\n";

sub checkBinaries {
  return "Cannot find a Tcl/Tk wish executable. Please install Tcl/Tk first!\n" unless -x $wish;

  my $sys = &mySystem;
  my $exe = "$install_dir/MODULES/bender$sys";
  return "Cannot find binary $exe!\n" unless -x $exe;

  my ($ldd, $any, $anyErr);
  if (($ldd = findImage('ldd'))) {
    # test for missing shared libraries
    open F, "$ldd $exe|";
    while (<F>) {
      next unless /([^ ]+)[\s]+=>[\s]+(.+)$/;
      $any = 1;
      my $lib = $1;
      my $rest = $2;
      next unless $lib =~ /^lib/;
      if ($isLinux) {
        $rest =~ /\((0x[0-9a-fA-F]+)\)/;
        $rest = $1;
        my $ok = $rest ne '';
        eval '$ok = 0 + ' . $ok if $ok;
        $anyErr .= "problems with library $lib needed for $exe!\n"
          unless $ok;
      } else {
        $rest =~ /^(\/[^\s]+)/;
        $rest = $1;
        $anyErr .=  "problems with library $lib needed for $exe!\n"
          unless $rest &&  -r $rest;
      }
    }
    close F;
  }
  return $anyErr;
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

sub myexit {
  print $_[0];
  exit(2);
}

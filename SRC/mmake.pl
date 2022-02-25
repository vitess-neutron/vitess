#!/usr/bin/perl
# Generate Vitess Makefile for both Unix gmake and Windows nmake
#
# see usage

use strict;

### configure here ####################################################
###

# unix comment
my $unixcomment =<<'EOS';
# compile hosts used at HZB
# Linux   : dixi3  openSUSE 10.2 (i586)
# Linux64 : dixi4 openSUSE 11.3 (x86_64)
# Darwin  : daisy.helmholtz-berlin.de 11.2.0 (X86-64)
EOS

###
### end configure ######################################################

my ($subdir, $g2sub, $g2subsub, $suse_version);

$_ = `grep VERSION /etc/SuSE-release 2>/dev/null`;
$suse_version = $1 if / = (.+)/;


### define targets #####################################################
###
# special object init handled implicitly for Makefile by the .c.o rule
#     and explicitly for vitess.mak
# it is special because it sees special definitions like the VITESS version
my @InitObj = qw(init);

# tool objects
my @Obj = qw(general convert intersection matrix sample softabort);

# modules which need TOOL (init general convert message)
my @C = qw(ascii2bin monitor1
	   mon2_div mon2_pos mon2_posdiv mon2_tofwl mon2_wldiv mon2_kdiv mon2_rdiv
	   mon_brilliance velselect read_in writeout gener_batch lattice_dist
	   mirror_coating surface_file gener_bispectral guide_shape spin_reset
           capture_flux runtime fom gener_pipe opt_sim);

# modules which need ITOOL (=TOOL + intersection)
my @CI = qw(chopper_disc chopper_fermi_parallel collimator
	    slit grid source spacewindow_multiple space lenses beamstop);

# modules which need MTOOL (=ITOOL + matrix)
my @CM = qw(detector eval_elast eval_elast2 eval_inelast eval_sans frame
	    monitorpol_1d monitorpol_pos
	    monochr_analyser
	    polariser_sm
	    polariser_he3 flipper_coil
	    pol_mirror
	    collimator_radial collimator_soller
	    precessionfield sesans_field
	    define_direction
	    cas_v40
	    mirror_elliptical
      flipper_gradient
      spacewindow
      rotating_field
      resonator_drabkin
          );

# modules NTOOL (= TOOL + mathvector mathmatrix)
my @CN = qw(monitor1D monitor2D);

# modules which need MGTOOL (=MTOOL + mathfunctions)
my @CMG = qw(guide_parallel monochromator);

# module which need GTOOL (=TOOL + mathvector mathfunctions)
my @CG = qw(guide_elliptic filter);

# modules which need STOOL (=MTOOL + sample)
my @CS = qw(sample_powder sample_s_q sample_sans sample_environment sample_nxs
	    sample_elasticisotr sample_inelast sample_reflectom sample_singcryst
);

my @Gexe = qw(bender visual sm_ensemble_parallel dist_time);

# auxillary programs which need no further libs
my @PTool = qw(chop_phases standard_deviation direct_view sortiap merge_spectra);

# modules with helper thread support
my @ParMod =  qw(chopper_fermi_parallel sm_ensemble_parallel);

#modules with mon_healder
my @MonMod = qw(mon2_div mon2_kdiv mon2_pos mon2_posdiv mon2_rdiv mon2_tofwl mon2_wldiv mon_brilliance monitor1 monitorpol_1d monitorpol_pos);

my %Macro;
$Macro{$_} = '$(TOOL)' foreach ('visual', 'dist_time', @C);
$Macro{$_} = '$(ITOOL)' foreach ('bender', @CI);
$Macro{$_} = '$(MTOOL)' foreach ('sm_ensemble_parallel', @CM);
$Macro{$_} = '$(NTOOL)' foreach (@CN);
$Macro{$_} = '$(GTOOL)' foreach (@CG);
$Macro{$_} = '$(MGTOOL)' foreach (@CMG);
$Macro{$_} = '$(STOOL)' foreach @CS;

my %dep = (			# needed objects for a module
	   source => 'src_modchar source_csns source_ess trace',
	   sample_s_q => 'sq_calc',
	   monochr_analyser => 'ma_functions ma_geom',
     monochromator => 'monochrclass mathvector mathmatrix',
	   gener_batch => 'gener_fct',
	   gener_pipe => 'pipe_fct',
	   opt_sim => 'opt_grad opt_grad_mc opt_metro opt_swarm opt_fct calc_sim_fom',
	   grid => 'bender_inter_data',
	   spacewindow => 'bender_inter_data',
	   spacewindow_multiple => 'bender_inter_data',
	   chopper_disc => 'bender_inter_data',
	   lenses => 'lensetr cpgplot',
	   mirror_elliptical => 'mirrrefl',
     sample_nxs => 'nxs sgclib sgfind sghkl sgio sgsi',
     monitor1D => 'mon2_header mon1D',
     monitor2D => 'mon2_header mon2D',
     read_in => 'mcpl trace',
     writeout => 'mcpl',
     guide_parallel => 'threadHelper mcpl'
	  );
$dep{$_} = 'threadHelper' foreach (@ParMod);
$dep{$_} = 'mon2_header' foreach (@MonMod);

# objects necessary for some modules, to be compiled separately
# remember in %K we already have these
# (omit cpgplot here, it should go to @Gobj, not @Obj

my %K;
foreach (keys %dep) {
  foreach (split(' ', $dep{$_})) {
    $K{$_} = 1 unless $_ eq 'cpgplot';
  }
}

push @Obj, keys(%K);

$dep{bender} = $_ = 'bendtest bendchtr bendertr bender_inter_data cpgplot';
# objects which have to be compiled with special options
my @Gobj;
foreach (split) {
  push @Gobj, $_ unless $K{$_};
}

$dep{$_} .= ' cpgplot' foreach qw(visual dist_time sm_ensemble_parallel);

my (%sopt, %lib);
foreach (qw(visual bender dist_time sm_ensemble_parallel lenses)) {
  $sopt{$_} = '$(GRAOPT)';      # special compile options for a module
  $lib{$_} = '$(GRALIB)';       # needed libs for a module
}

# Modules needing thread support:
my %Thread;
$Thread{$_} = 1 foreach @ParMod;


my @All = (@C, @CI, @CM, @CN, @CG, @CMG, @CS, @Gexe, @PTool);

###
### end define targets #####################################################################

my $makefile = 'Makefile.template';
my $nmakefile = 'vitess.mak';

sub usage {

  print <<EOS;
usage:
mmake.pl \{option\}
  this script is meant to be run under Unix, to generate two files
  1) Makfile suitable for GNU make
  2) vitess.mak suitable for Windows nmake
  mmake.pl tries to adopt Makefile to the local Unix system.

  Options may be
  -v                    be more verbose
       concering Unix make
  --mfile filename      generate makefile filename, default $makefile
  --lpath path          Unix: additional path to look for libraries, especially libgd, libz, libfreetype
                        separate multiple directories by :

       concerning Windows nmake
  --nfile filename      generate nmake filename, default $nmakefile, use /dev/null to ignore
                        if you do not give a path here, no nmake file will be generated
EOS

  exit;
}

my ($lpath, $sys, $arch, $s, $verbose, @LPath, %LPath);
my $libpng = 'png'; # unless changed by checkLibs
my $args = "@_";

while ($_ = shift) {
  if ($_ eq '-v') {
    $verbose = 1;
    next;
  }
  &usage unless /^--(.+)$/;
  $_ = $1;
  my $arg = shift;
  if ($_ eq 'mfile') {
    $makefile = $arg;
  } elsif ($_ eq 'nfile') {
    $nmakefile = $arg;
  } elsif ($_ eq 'lpath') {
    $lpath = $arg;
  } else {
    &usage;
  }
}

# try to read VITESS version from ../GUI/control.tcl

my ($version, $fullversion);
open F, '../GUI/control.tcl';
while (<F>) {
  if (/set t \"VITESS ([0-9.a-z]+)/) {
    $version = $fullversion = $1;
    last;
  }
}
close F;


&prepareMakefile if $makefile ne '';
&prepareNMakefile if $nmakefile ne '';


sub prepareMakefile {

  my ($xlib);

  $sys = getRes('uname');

  if ($sys eq 'Darwin') {
    $arch = getRes('uname -m');
  } else {
    $arch = getRes('uname -i');
  }

  if ( $arch eq 'x86_64') {
    $subdir = "${sys}_$arch";
    $xlib = 'lib64';
  } else {
    $subdir = $sys;
    $xlib = 'lib';
  }

  # candidates for libraries are
  $LPath{$_} = 1 foreach ('/usr/local/lib', '/usr/X11/lib', '/lib64', "/usr/X11R6/$xlib");

  $LPath{$_} = 1 foreach split ':', $lpath;
  # gather pathes which do exist
  foreach (keys %LPath) {
    if (-d $_) {
      my @A = glob "$_/*";
      push @LPath, $_ if @A > 0;
    }
  }

  return unless &checkLibs;

  open OF, ">$makefile";

  my $pwd = $ENV{PWD};
  $_ = join(" \\\n", sort @All);
  print OF <<EOS;
# Unix gmake Makefile for VITESS modules
# generated by mmake.pl $args
#
$unixcomment
INSTDIR = ../MODULES/
SUBDIR = $subdir

ALL = $_
EOS

  print OF <<'EOS';

TOOL = init.o general.o convert.o message.o softabort.o
ITOOL = intersection.o $(TOOL)
MTOOL = matrix.o $(ITOOL)
MGTOOL = mathfunctions.o $(MTOOL)
STOOL = sample.o $(MTOOL)
NTOOL = mathvector.o mathmatrix.o $(TOOL)
GTOOL = mathvector.o mathfunctions.o $(TOOL)

#Compile
EOS

  # set appropriate make macros
  $_ = ($sys eq 'Darwin' ? '' : '-s ') .
       '-O3 -Wall -Wpointer-arith -Wcast-qual -Wwrite-strings -fomit-frame-pointer -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Irng';

  print OF <<EOS;
CCOMP = gcc
CPLUSCOMP = g++
CFLAGS = -pthread $_

# alternative intel icc compiler
# uncomment the appropriate lines to use icc
#CCOMP = icc
#CPLUSCOMP = icc -x c++
#CFLAGS = -pthread -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -w3 -g0 -Wall -Wremarks -Irng -ffreestanding

CC = \$(CCOMP) \$(CFLAGS)
CPLUS = \$(CPLUSCOMP) \$(CFLAGS)

LIBS = -Lrng/$subdir -lgslran -lstdc++ -lm
GDOPEN = g2_open_gd
EOS

  $_ = $g2subsub;
  print OF "GRALIB = -DDO_PNG -DDO_X11 -DDO_GD -DVT_GRAPH -I. -Lrng/$subdir -lgslran -I$_ -L$_";
  print OF " -L$_" foreach @LPath;
  print OF " -lX11 -lg2 -lgd -l$libpng -lz -lfreetype -lXpm";
  print OF ' -lttf' if $sys ne 'Darwin' && $suse_version ne '' && $suse_version < 13;
  print OF ' -lm';

  print OF <<'EOS';


.KEEP_STATE :

all: $(ALL)

EOS

  $dep{$_} = "$_ $dep{$_}" foreach @All;

  foreach my $d (sort keys %dep) {
    my $l = $lib{$d} || '$(LIBS)';
    my ($ccomp, $srces) = translateDep(split(' ', $dep{$d}));

    print OF $d, ' : ', $srces, " $Macro{$d}\n\t", $ccomp, ($Thread{$d} ? ' -pthread' : ''),
        ' -o $@ $^ ', "$l\n\n";
  }

  if ($fullversion) {
    my @T = gmtime time;
    my $y = 1900 + $T[5];
    my $m = 1 + $T[4];
    my $d = $T[3];
    $_ = "$fullversion $subdir $y-$m-$d";
    print OF "init.o: init.c\n\t\$(CC) -DVVERS='\"$_\"' -c \$<\n"
  }

  print OF <<'EOS';
.c.o:
	$(CC) -c $<
.cpp.o:
	$(CPLUS) -c $<

install:
	a=_$(SUBDIR) ; h=$(INSTDIR) ; for l in $(ALL) ; do mv $$l $$h$$l$$a ; done
	-rm -f *.o
Copy:
	a=_$(SUBDIR) ; h=$(INSTDIR) ; for l in $(ALL) ; do cp $$l $$h$$l$$a ; done

clean :
	-rm -f *.o $(ALL)
EOS

  close OF;
  print STDERR <<EOS;
generated $makefile
    to compile VITESS sources here, do
    make
       to compile VITESS sources
    make install
       to move the executables to ../MODULES
EOS

}


sub prepareNMakefile {

  # Generate a Windows NMAKE file
  # Separate path parts with | here, will be substituted for Windows later

  open OF, ">$nmakefile";

  $s = <<'EOS';
# Vitess NMAKE File
# to be used as
# nmake /f vitess.mak all
# after cd to the SRC directory of the Vitess tree
# from a Microsoft VS 2017 cmd.exe
SPATH=.
GPATH=.\g2-0.72
GSLPATH=.\rng

OD=.\Release
IDIR=.\Release

CPP=cl.exe
DEFS=/DNDEBUG /DDO_WIN32 /DCONSOLE /DWIN32 /D "_MBCS" /D_CRT_SECURE_NO_WARNINGS
INC=/I "$(SPATH)" /I "$(GSLPATH)"
CPP_OPT=/nologo /MT /W3 /Ox /Oy /GF $(INC) $(DEFS) /Fp"$(IDIR)\vit.pch" /FD /EHsc /c
CPP_PROJ=$(CPP_OPT) /Fo"$(IDIR)\\" /Fd"$(IDIR)\\"
GRAOPT=/I "$(GPATH)" /I "$(GPATH)\WIN32" /I "$(GPATH)\PS" /DDO_PS /DVT_GRAPH
LIBGSL=libgsl.lib

LINK32=link.exe
WINLIBS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib
LINK32_FLAGS=/nologo /subsystem:console /incremental:no /machine:I386 /opt:ref /opt:icf,5 \
 /libpath:"$(GPATH)" /libpath:"$(GSLPATH)"
TOOL="$(IDIR)\init.obj" "$(IDIR)\general.obj" "$(IDIR)\convert.obj" "$(IDIR)\message.obj" "$(IDIR)\softabort.obj"
ITOOL="$(IDIR)\intersection.obj" $(TOOL)
MTOOL="$(IDIR)\matrix.obj" $(ITOOL)
NTOOL="$(IDIR)\mathvector.obj" "$(IDIR)\mathmatrix.obj" "$(IDIR)\mon2D.obj" $(TOOL)
GTOOL="$(IDIR)\mathvector.obj" "$(IDIR)\mathfunctions.obj" $(TOOL)
MGTOOL="$(IDIR)\mathfunctions.obj" $(MTOOL)
STOOL="$(IDIR)\sample.obj" $(MTOOL)
GRALIB=libg2.lib

ML= /NODEFAULTLIB:libc.lib $(WINLIBS) $(LIBGSL) $(LINK32_FLAGS)
ML_T= /NODEFAULTLIB:libc.lib $(WINLIBS) $(LIBGSL) $(LINK32_FLAGS)

.c{$(IDIR)}.obj::
	$(CPP) @<<
	$(CPP_PROJ) $<
<<

.cpp{$(IDIR)}.obj::
	$(CPP) @<<
	$(CPP_PROJ) $<
<<

"$(OD)" :
	if not exist "$(OD)\$(NULL)" mkdir "$(OD)"

EOS

  $s .= 'ALL :';
  foreach (@All) {
    $s .= " |\n\t" . '"$(OD)|' . $_ . '.exe"';
  }

  $s .= "\n\n";

  my $rulestart = "SOURCE=\$(SPATH)|xxx.c\n" . <<'EOS';
"$(IDIR)|xxx.obj" : $(SOURCE)
	$(CPP) $(CPP_PROJ) $(SOURCE)

EOS

  $version =~ /([0-9]+)\.([0-9]+)/;
  my $rule = <<EOS;
SOURCE=\$(SPATH)|xxx.c
"\$(IDIR)|xxx.obj" : \$(SOURCE)
	\$(CPP) /DVMAJOR=$1 /DVMINOR=$2 \$(CPP_PROJ) \$(SOURCE)
EOS
  subRule($rule, @InitObj);

  # Objects
  $rule = $rulestart;
  subRule($rule, @Obj);

  # Tool, @C
  $rule .=  <<'EOS';
"$(OD)|xxx.exe" : "$(OD)" $(TOOL) "$(OD)|xxx.obj" yyy
	$(LINK32) $(ML) /pdb:"$(OD)|xxx.pdb" /out:"$(OD)|xxx.exe" "$(IDIR)|xxx.obj" $(TOOL) yyy zzz

EOS
  subRule($rule, @C);

  # ITool, @CI
  $rule =~ s/TOOL/ITOOL/g;
  subRule($rule, @CI);

  # MTool, @CM
  $rule =~ s/ITOOL/MTOOL/g;
  subRule($rule, @CM);

  # NTool, @CN
  $rule =~ s/MTOOL/NTOOL/g;
  subRule($rule, @CN);

  # GTool, @CG
  $rule =~ s/NTOOL/GTOOL/g;
  subRule($rule, @CG);

  # MGTool, @CMG
  $rule =~ s/GTOOL/MGTOOL/g;
  subRule($rule, @CMG);

  # STool, @CS
  $rule =~ s/MGTOOL/STOOL/g;
  subRule($rule, @CS);

  # PTool
  $rule = $rulestart . <<'EOS';
"$(OD)|xxx.exe" : "$(OD)" "$(OD)|xxx.obj" yyy
	$(LINK32) $(ML) /pdb:"$(OD)|xxx.pdb" /out:"$(OD)|xxx.exe" "$(IDIR)|xxx.obj" yyy

EOS
  subRule($rule, @PTool);

  # Gobj
  $rule = 'SOURCE=$(SPATH)|xxx.c' . "\n" . <<'EOS';
"$(IDIR)|xxx.obj" : $(SOURCE)
	$(CPP) $(GRAOPT) $(CPP_PROJ) $(SOURCE)

EOS

  subRule($rule, @Gobj);

  $rule .= <<'EOS';
"$(OD)|xxx.exe" : "$(OD)" "$(OD)|xxx.obj" $(MTOOL) yyy
	$(LINK32) $(ML) $(MTOOL) $(GRALIB) /pdb:"$(OD)|xxx.pdb" /out:"$(OD)|xxx.exe" "$(IDIR)|xxx.obj" yyy

EOS
  subRule($rule, @Gexe);

  # change to Windows text conventions: use backslash as path separator, reduce blanks
  foreach (split "\n", $s) {
    s/ +/ /;
    s/ $//;
    tr/|/\\/;
    print OF "$_\n";
  }
  close OF;

  print STDERR <<EOS;

generated $nmakefile
    to compile VITESS sources under Windows,
         copy $nmakefile to your windows host SRC directory,
         start a Visual Studio 2017 command shell there,
         cd to the SRC directory, and type
         nmake /f $nmakefile all
EOS
}

sub subRule {
  # substitute names in rules
  # xxx  module name
  # ooo  special cc options
  # yyy  objects to link with
  # zzz  libraries to link with

  my $rule = shift;
  foreach my $c (@_) {
    $_ = $rule;
    if (-s "$c.cpp") {
      # cpp Quelle
      s/xxx\.c/$c.cpp/g;
    }
    s/xxx/$c/g;
    my $d = $dep{$c};
    if ($d eq '') {
      s/yyy//g;
    } else {
      my ($rep, %M);
      $M{$_} = 1 foreach split ' ',$d;
      $M{$c} = 0;
      foreach (keys %M) {
        $rep .= '"$(OD)|' . $_ . '.obj" ' if $M{$_};
      }
      s/yyy/$rep/g;
    }
    s/zzz/$lib{$c}/;
    s/ooo/$sopt{$c}/;
    s/\(ML\)/\(ML_T\)/g if $Thread{$c};
    $s .= $_;
  }
}

sub getRes {
  $_ = `$_[0]`;
  chomp;
  $_;
}

sub checkLibs {
  # look for gcc and libraries; warn if not found

  $_ = `which gcc`;
  chomp;
  unless ( -X $_) {
    print STDERR "could not locate gcc\n";
    return 0;
  }

  my $ext = 'so';
  my @Places = @LPath;
  $_ = '/usr/lib';
  if ($sys eq 'Darwin') {
    $ext = 'dylib';
    push @Places, '/usr/lib';
  } elsif ($sys eq 'Linux') {
    if ($arch eq 'x86_64') {
      push @Places, '/usr/lib64', '/lib64';
    } else {
      push @Places, '/usr/lib';
    }
  } else {
    print STDERR "$sys is not known here\n";
    return 0;                    # no further checks for unknown systems
  }

  my $anyerr;

  # look for libg2.a
  $g2sub = './g2-0.72';
  $g2subsub = $_ = "$g2sub/$subdir";
  $_ .= '/libg2.a';
  unless (-s $_) {
    print STDERR "no g2 library $_ found. Read $g2sub/Readme.vitess for tipps how to compile it.\n";
    return 0;
  }

  my @Needlib = qw(X11 gd png z freetype Xpm);

  # we need libttf for SuSE versions older than 13
  push  @Needlib, 'ttf' if $sys ne 'Darwin' && $suse_version ne '' && $suse_version < 13;

  foreach my $lib (@Needlib) {
    my $found = 0;
    my $lname = "lib$lib.$ext";
    foreach (@Places) {
      my $n = "$_/$lname";
      print "look for $n\n" if $verbose;
      if (-e $n) {
        $found = 1;
        last;
      }
    }
    next if $found;
    if ($lib eq 'png') {
      # hack: look for libpng14
      print STDERR "\tlooking for libpng14, $lname not found\n";
      $lname = "libpng14.$ext";
      foreach (@Places) {
        if (-s "$_/$lname") {
          $libpng = 'png14';
          $found = 1;
          last;
        }
      }
      next if $found;
    }
    print STDERR "could not locate $lname\n";
    $anyerr = 1;
  }
  return 1 unless $anyerr;
  if ($sys eq 'Darwin') {
    print STDERR "read gnuplot_darwin.txt for tips to install needed tools\n";
  } elsif ($sys eq 'Linux') {
    if ($suse_version) {
      $_ = $suse_version < 13 ? ' ,libttf' : '';
      print STDERR <<EOS;
Use yast2 to search & install packages!
VITESS needs tk, gnuplot, libgd, libfreetype$_, libzlib, libpng, libXpm.
If something is missing after installing these, try to add the -devel
packages of libs, like libgd-devel, libpng-devel.
EOS
    } else {
      print STDERR <<EOS;
Use your Linux distribution tool to search & install packages!
VITESS needs tk, gnuplot, libgd, libfreetype, libzlib, libpng, libXpm.
If something is missing after installing these, try to add the developer
packages of libs.
EOS
    }
  }
  0;
}

sub translateDep {
  my $all_cc = 1;
  my ($s, $fn, @F);
  foreach (@_) {
    $fn = "$_.cpp";
    if (-s $fn) {
      $all_cc = 0;
    } else {
      $fn = "$_.c";
      myexit("$fn missing") unless -s $fn;
    }
    push @F, $fn;
  }
  return ($all_cc ? '$(CC)' : '$(CPLUS)', join(' ', @F));
}

sub myexit {
  print "$_[0]!\n";
  exit 0;
}

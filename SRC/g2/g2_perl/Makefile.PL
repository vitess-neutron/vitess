# Generated automatically from Makefile.PL.in by configure.
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'	=> 'G2',
    'VERSION_FROM' => 'G2.pm', # finds $VERSION
    'LIBS'	=> ['-L./.. -lg2.0.40 -L/net/usr/lib  -L/usr/local/lib -lm -lX11 -lgd'],  # e.g., '-lm' 
    'DEFINE'	=> ' -DUNIX=1 -DDO_PS=1 -DDO_X11=1 -DDO_GIF=1 -DSTDC_HEADERS=1 -DHAVE_LIMITS_H=1 ',                    # e.g., '-DHAVE_SOMETHING' 
    'INC'	=> '-I./../include',               # e.g., '-I/usr/local/include' 
);

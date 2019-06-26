# Generated automatically from Makefile.in by configure.
#
#
# Makefile for g2 library
#
#

#
# g2 installation directories
#
LIBDIR = /usr/local/lib
INCDIR = /usr/local/include

#LIBDIR = $(HOME)/local/lib
#INCDIR = $(HOME)/local/include

#
#------------------------ do not edit ------------------------
#
SHELL = /bin/sh

CC           = gcc
CFLAGS       = -I./src -I/net/usr/include  -I/usr/local/include  -DUNIX=1 -DDO_PS=1 -DDO_X11=1 -DDO_GIF=1 -DSTDC_HEADERS=1 -DHAVE_LIMITS_H=1 
INSTALL      = /usr/bin/installbsd -c
INSTALL_DATA = ${INSTALL} -m 644
FIND         = find
MAKEDEPEND   = makedepend
AR           = ar
ARFLAGS      = -rs

BASE_DIR = ./src
BASE_SRC = $(BASE_DIR)/g2_device.c         $(BASE_DIR)/g2_ui_control.c \
           $(BASE_DIR)/g2_util.c           $(BASE_DIR)/g2_fif.c \
           $(BASE_DIR)/g2_virtual_device.c $(BASE_DIR)/g2_physical_device.c \
           $(BASE_DIR)/g2_graphic_pd.c     $(BASE_DIR)/g2_control_pd.c \
           $(BASE_DIR)/g2_ui_graphic.c     $(BASE_DIR)/g2_ui_virtual_device.c \
           $(BASE_DIR)/g2_ui_device.c
BASE_INS = $(BASE_DIR)/g2.h


PS_DIR = ./src/PS
PS_SRC = $(PS_DIR)/g2_PS.c
PS_INS = $(PS_DIR)/g2_PS.h


X11_DIR = ./src/X11
X11_SRC = $(X11_DIR)/g2_X11.c
X11_INS = $(X11_DIR)/g2_X11.h


GIF_DIR = ./src/GIF
GIF_SRC = $(GIF_DIR)/g2_GIF.c
GIF_INS = $(GIF_DIR)/g2_GIF.h


SRC = $(BASE_SRC) $(PS_SRC) $(X11_SRC) $(GIF_SRC)
OBJ = $(SRC:.c=.o)

INS =  $(BASE_INS) $(PS_INS) $(X11_INS) $(GIF_INS)

.c.o:  
	$(CC) $(CFLAGS) -c $< -o $@


all: libg2.a
	test -d ./include || mkdir ./include
	cp $(INS) ./include/

shared: libg2.so
	@echo

libg2.a: $(OBJ)
	$(AR) $(ARFLAGS) libg2.a $(OBJ)
	ln -s libg2.a libg2.0.40.a

libg2.so: $(OBJ)
	$(CC) -shared -o libg2.so $(OBJ) -lc -lm

install: libg2.a
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	test -d $(INCDIR) || mkdir -p $(INCDIR)
	$(INSTALL_DATA) libg2.a $(LIBDIR)
	for IFILE in $(INS); do \
		$(INSTALL_DATA) $$IFILE  $(INCDIR); \
	done

clean:
	-(cd ./demo ; make clean)
	-rm -f $(OBJ)
	-rm -f libg2.a config.cache config.log Makefile.bak config.status
	-rm -f ./include/*.h
	-$(FIND) . -name "*~" -exec rm -f {} \;
	-(cd ./g2_perl ; make clean)
	-rm -f ./g2_perl/test.ps ./g2_perl/test.gif
	-rm -f ./libg2.0.40.a

demo:	libg2.a
	(cd ./demo ; make)

depend:
	$(MAKEDEPEND) -- $(CFLAGS) -- $(SRC)
	@echo
	@echo "Run 'make' to build g2 library."
	@echo


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.

./src/g2_device.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_device.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_device.o: /usr/include/sys/types.h
./src/g2_device.o: /usr/include/mach/machine/vm_types.h
./src/g2_device.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_device.o: /usr/include/sys/limits.h
./src/g2_device.o: /usr/include/sys/machine/machlimits.h
./src/g2_device.o: /usr/include/sys/syslimits.h
./src/g2_device.o: /usr/include/sys/machine/machtime.h
./src/g2_device.o: /usr/include/sys/rt_limits.h ./src/g2.h ./src/g2_device.h
./src/g2_device.o: ./src/g2_physical_device.h ./src/g2_funix.h
./src/g2_device.o: ./src/g2_virtual_device.h ./src/g2_util.h
./src/g2_device.o: /usr/include/stdlib.h
./src/g2_ui_control.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_ui_control.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_ui_control.o: /usr/include/sys/types.h
./src/g2_ui_control.o: /usr/include/mach/machine/vm_types.h
./src/g2_ui_control.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_ui_control.o: /usr/include/sys/limits.h
./src/g2_ui_control.o: /usr/include/sys/machine/machlimits.h
./src/g2_ui_control.o: /usr/include/sys/syslimits.h
./src/g2_ui_control.o: /usr/include/sys/machine/machtime.h
./src/g2_ui_control.o: /usr/include/sys/rt_limits.h /usr/include/stdlib.h
./src/g2_ui_control.o: /usr/include/math.h ./src/g2.h ./src/g2_device.h
./src/g2_ui_control.o: ./src/g2_physical_device.h ./src/g2_funix.h
./src/g2_ui_control.o: ./src/g2_virtual_device.h ./src/g2_control_pd.h
./src/g2_util.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_util.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_util.o: /usr/include/sys/types.h
./src/g2_util.o: /usr/include/mach/machine/vm_types.h
./src/g2_util.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_util.o: /usr/include/sys/limits.h
./src/g2_util.o: /usr/include/sys/machine/machlimits.h
./src/g2_util.o: /usr/include/sys/syslimits.h
./src/g2_util.o: /usr/include/sys/machine/machtime.h
./src/g2_util.o: /usr/include/sys/rt_limits.h /usr/include/stdlib.h
./src/g2_util.o: /usr/include/math.h ./src/g2_util.h
./src/g2_util.o: ./src/g2_physical_device.h ./src/g2.h ./src/g2_funix.h
./src/g2_fif.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_fif.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_fif.o: /usr/include/sys/types.h /usr/include/mach/machine/vm_types.h
./src/g2_fif.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_fif.o: /usr/include/sys/limits.h
./src/g2_fif.o: /usr/include/sys/machine/machlimits.h
./src/g2_fif.o: /usr/include/sys/syslimits.h
./src/g2_fif.o: /usr/include/sys/machine/machtime.h
./src/g2_fif.o: /usr/include/sys/rt_limits.h /usr/include/string.h
./src/g2_fif.o: /usr/include/strings.h ./src/g2.h ./src/g2_util.h
./src/g2_fif.o: /usr/include/stdlib.h ./src/g2_physical_device.h
./src/g2_fif.o: ./src/g2_funix.h ./src/PS/g2_PS.h ./src/X11/g2_X11.h
./src/g2_fif.o: ./src/GIF/g2_GIF.h
./src/g2_virtual_device.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_virtual_device.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_virtual_device.o: /usr/include/sys/types.h
./src/g2_virtual_device.o: /usr/include/mach/machine/vm_types.h
./src/g2_virtual_device.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_virtual_device.o: /usr/include/sys/limits.h
./src/g2_virtual_device.o: /usr/include/sys/machine/machlimits.h
./src/g2_virtual_device.o: /usr/include/sys/syslimits.h
./src/g2_virtual_device.o: /usr/include/sys/machine/machtime.h
./src/g2_virtual_device.o: /usr/include/sys/rt_limits.h
./src/g2_virtual_device.o: ./src/g2_virtual_device.h ./src/g2_device.h
./src/g2_virtual_device.o: ./src/g2_physical_device.h ./src/g2.h
./src/g2_virtual_device.o: ./src/g2_funix.h ./src/g2_util.h
./src/g2_virtual_device.o: /usr/include/stdlib.h
./src/g2_physical_device.o: /usr/include/stdlib.h /usr/include/standards.h
./src/g2_physical_device.o: /usr/include/getopt.h /usr/include/sys/types.h
./src/g2_physical_device.o: /usr/include/mach/machine/vm_types.h
./src/g2_physical_device.o: /usr/include/sys/select.h /usr/include/stdio.h
./src/g2_physical_device.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_physical_device.o: /usr/include/sys/limits.h
./src/g2_physical_device.o: /usr/include/sys/machine/machlimits.h
./src/g2_physical_device.o: /usr/include/sys/syslimits.h
./src/g2_physical_device.o: /usr/include/sys/machine/machtime.h
./src/g2_physical_device.o: /usr/include/sys/rt_limits.h
./src/g2_physical_device.o: ./src/g2_physical_device.h ./src/g2.h
./src/g2_physical_device.o: ./src/g2_funix.h ./src/g2_util.h
./src/g2_graphic_pd.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_graphic_pd.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_graphic_pd.o: /usr/include/sys/types.h
./src/g2_graphic_pd.o: /usr/include/mach/machine/vm_types.h
./src/g2_graphic_pd.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_graphic_pd.o: /usr/include/sys/limits.h
./src/g2_graphic_pd.o: /usr/include/sys/machine/machlimits.h
./src/g2_graphic_pd.o: /usr/include/sys/syslimits.h
./src/g2_graphic_pd.o: /usr/include/sys/machine/machtime.h
./src/g2_graphic_pd.o: /usr/include/sys/rt_limits.h /usr/include/stdlib.h
./src/g2_graphic_pd.o: /usr/include/math.h ./src/g2_funix.h
./src/g2_graphic_pd.o: ./src/g2_virtual_device.h ./src/g2_graphic_pd.h
./src/g2_graphic_pd.o: ./src/g2_physical_device.h ./src/g2.h
./src/g2_graphic_pd.o: ./src/g2_control_pd.h ./src/g2_util.h
./src/g2_control_pd.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_control_pd.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_control_pd.o: /usr/include/sys/types.h
./src/g2_control_pd.o: /usr/include/mach/machine/vm_types.h
./src/g2_control_pd.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_control_pd.o: /usr/include/sys/limits.h
./src/g2_control_pd.o: /usr/include/sys/machine/machlimits.h
./src/g2_control_pd.o: /usr/include/sys/syslimits.h
./src/g2_control_pd.o: /usr/include/sys/machine/machtime.h
./src/g2_control_pd.o: /usr/include/sys/rt_limits.h /usr/include/stdlib.h
./src/g2_control_pd.o: /usr/include/math.h ./src/g2.h ./src/g2_device.h
./src/g2_control_pd.o: ./src/g2_physical_device.h ./src/g2_funix.h
./src/g2_control_pd.o: ./src/g2_virtual_device.h ./src/g2_util.h
./src/g2_ui_graphic.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_ui_graphic.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_ui_graphic.o: /usr/include/sys/types.h
./src/g2_ui_graphic.o: /usr/include/mach/machine/vm_types.h
./src/g2_ui_graphic.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_ui_graphic.o: /usr/include/sys/limits.h
./src/g2_ui_graphic.o: /usr/include/sys/machine/machlimits.h
./src/g2_ui_graphic.o: /usr/include/sys/syslimits.h
./src/g2_ui_graphic.o: /usr/include/sys/machine/machtime.h
./src/g2_ui_graphic.o: /usr/include/sys/rt_limits.h ./src/g2_device.h
./src/g2_ui_graphic.o: ./src/g2_physical_device.h ./src/g2.h ./src/g2_funix.h
./src/g2_ui_graphic.o: ./src/g2_virtual_device.h ./src/g2_graphic_pd.h
./src/g2_ui_graphic.o: ./src/g2_util.h /usr/include/stdlib.h
./src/g2_ui_virtual_device.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_ui_virtual_device.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_ui_virtual_device.o: /usr/include/sys/types.h
./src/g2_ui_virtual_device.o: /usr/include/mach/machine/vm_types.h
./src/g2_ui_virtual_device.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_ui_virtual_device.o: /usr/include/sys/limits.h
./src/g2_ui_virtual_device.o: /usr/include/sys/machine/machlimits.h
./src/g2_ui_virtual_device.o: /usr/include/sys/syslimits.h
./src/g2_ui_virtual_device.o: /usr/include/sys/machine/machtime.h
./src/g2_ui_virtual_device.o: /usr/include/sys/rt_limits.h ./src/g2.h
./src/g2_ui_virtual_device.o: ./src/g2_device.h ./src/g2_physical_device.h
./src/g2_ui_virtual_device.o: ./src/g2_funix.h ./src/g2_virtual_device.h
./src/g2_ui_virtual_device.o: ./src/g2_util.h /usr/include/stdlib.h
./src/g2_ui_device.o: /usr/include/stdio.h /usr/include/standards.h
./src/g2_ui_device.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/g2_ui_device.o: /usr/include/sys/types.h
./src/g2_ui_device.o: /usr/include/mach/machine/vm_types.h
./src/g2_ui_device.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/g2_ui_device.o: /usr/include/sys/limits.h
./src/g2_ui_device.o: /usr/include/sys/machine/machlimits.h
./src/g2_ui_device.o: /usr/include/sys/syslimits.h
./src/g2_ui_device.o: /usr/include/sys/machine/machtime.h
./src/g2_ui_device.o: /usr/include/sys/rt_limits.h ./src/g2.h
./src/g2_ui_device.o: ./src/g2_funix.h ./src/g2_device.h
./src/g2_ui_device.o: ./src/g2_physical_device.h ./src/g2_virtual_device.h
./src/PS/g2_PS.o: /usr/include/stdio.h /usr/include/standards.h
./src/PS/g2_PS.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/PS/g2_PS.o: /usr/include/sys/types.h
./src/PS/g2_PS.o: /usr/include/mach/machine/vm_types.h
./src/PS/g2_PS.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/PS/g2_PS.o: /usr/include/sys/limits.h
./src/PS/g2_PS.o: /usr/include/sys/machine/machlimits.h
./src/PS/g2_PS.o: /usr/include/sys/syslimits.h
./src/PS/g2_PS.o: /usr/include/sys/machine/machtime.h
./src/PS/g2_PS.o: /usr/include/sys/rt_limits.h /usr/include/stdlib.h
./src/PS/g2_PS.o: /usr/include/stdarg.h /usr/include/limits.h ./src/g2.h
./src/PS/g2_PS.o: ./src/g2_device.h ./src/g2_physical_device.h
./src/PS/g2_PS.o: ./src/g2_funix.h ./src/g2_virtual_device.h ./src/g2_util.h
./src/PS/g2_PS.o: ./src/g2_config.h ./src/PS/g2_PS.h ./src/PS/g2_PS_P.h
./src/PS/g2_PS.o: ./src/PS/g2_PS_funix.h ./src/PS/g2_PS_definitions.h
./src/X11/g2_X11.o: /usr/include/stdio.h /usr/include/standards.h
./src/X11/g2_X11.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/X11/g2_X11.o: /usr/include/sys/types.h
./src/X11/g2_X11.o: /usr/include/mach/machine/vm_types.h
./src/X11/g2_X11.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/X11/g2_X11.o: /usr/include/sys/limits.h
./src/X11/g2_X11.o: /usr/include/sys/machine/machlimits.h
./src/X11/g2_X11.o: /usr/include/sys/syslimits.h
./src/X11/g2_X11.o: /usr/include/sys/machine/machtime.h
./src/X11/g2_X11.o: /usr/include/sys/rt_limits.h /usr/include/stdlib.h
./src/X11/g2_X11.o: /usr/include/stdarg.h /usr/include/limits.h
./src/X11/g2_X11.o: /usr/include/math.h /usr/include/string.h
./src/X11/g2_X11.o: /usr/include/strings.h /usr/include/X11/Xlib.h
./src/X11/g2_X11.o: /usr/include/X11/X.h /usr/include/X11/Xfuncproto.h
./src/X11/g2_X11.o: /usr/include/X11/Xosdefs.h /usr/include/stddef.h
./src/X11/g2_X11.o: /usr/include/X11/Xutil.h ./src/g2_device.h
./src/X11/g2_X11.o: ./src/g2_physical_device.h ./src/g2.h ./src/g2_funix.h
./src/X11/g2_X11.o: ./src/g2_virtual_device.h ./src/g2_util.h
./src/X11/g2_X11.o: ./src/X11/g2_X11_P.h ./src/X11/g2_X11.h
./src/X11/g2_X11.o: ./src/X11/g2_X11_funix.h ./src/g2_config.h
./src/GIF/g2_GIF.o: /usr/include/stdio.h /usr/include/standards.h
./src/GIF/g2_GIF.o: /usr/include/sys/seek.h /usr/include/va_list.h
./src/GIF/g2_GIF.o: /usr/include/sys/types.h
./src/GIF/g2_GIF.o: /usr/include/mach/machine/vm_types.h
./src/GIF/g2_GIF.o: /usr/include/sys/select.h /usr/include/getopt.h
./src/GIF/g2_GIF.o: /usr/include/sys/limits.h
./src/GIF/g2_GIF.o: /usr/include/sys/machine/machlimits.h
./src/GIF/g2_GIF.o: /usr/include/sys/syslimits.h
./src/GIF/g2_GIF.o: /usr/include/sys/machine/machtime.h
./src/GIF/g2_GIF.o: /usr/include/sys/rt_limits.h /usr/include/stdlib.h
./src/GIF/g2_GIF.o: /usr/include/math.h ./src/g2.h ./src/g2_device.h
./src/GIF/g2_GIF.o: ./src/g2_physical_device.h ./src/g2_funix.h
./src/GIF/g2_GIF.o: ./src/g2_virtual_device.h ./src/g2_util.h
./src/GIF/g2_GIF.o: ./src/g2_config.h ./src/GIF/g2_GIF_P.h
./src/GIF/g2_GIF.o: /net/usr/include/gd.h /net/usr/include/gd_io.h
./src/GIF/g2_GIF.o: /net/usr/include/gdfontt.h /net/usr/include/gdfonts.h
./src/GIF/g2_GIF.o: /net/usr/include/gdfontmb.h /net/usr/include/gdfontl.h
./src/GIF/g2_GIF.o: /net/usr/include/gdfontg.h ./src/GIF/g2_GIF.h
./src/GIF/g2_GIF.o: ./src/GIF/g2_GIF_funix.h

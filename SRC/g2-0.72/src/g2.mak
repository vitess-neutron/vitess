# nmake Makefile for g2 library

G2_VERSION = 0.72

CC           = cl
CFLAGS       = /I.\ -O2 /DDO_PS=1 /DDO_WIN=1 /DDO_GD=1 /DDO_GIF=1 \
 /DSTDC_HEADERS=1 /DHAVE_SYS_TYPES_H=1 /DHAVE_SYS_STAT_H=1 /DHAVE_STDLIB_H=1 \
 /DHAVE_STRING_H=1 /DHAVE_MEMORY_H=1 /DHAVE_STRINGS_H=1 /DHAVE_INTTYPES_H=1 /DHAVE_STDINT_H=1 \
 /DHAVE_UNISTD_H=1 /DHAVE_LIMITS_H=1 /c

AR            = lib

BDIR= .
OBJS= .\g2_device.obj .\g2_ui_control.obj \
      .\g2_util.obj .\g2_fif.obj \
      .\g2_virtual_device.obj .\g2_physical_device.obj \
      .\g2_graphic_pd.obj .\g2_control_pd.obj \
      .\g2_ui_graphic.obj .\g2_ui_virtual_device.obj \
      .\g2_ui_device.obj .\g2_splines.obj
      
BASE_INS= $(BDIR)\g2.h

PS_DIR= $(BDIR)\PS
PS_SRC= $(PS_DIR)\g2_PS.c
PS_OBJ= $(PS_DIR)\g2_PS.obj
PS_INS= $(PS_DIR)\g2_PS.h

WIN32_DIR= $(BDIR)\WIN32
WIN32_SRC= $(WIN32_DIR)/g2_win32.c $(WIN32_DIR)/g2_win32_thread.c 
WIN32_OBJ= $(WIN32_DIR)/g2_win32.obj $(WIN32_DIR)/g2_win32_thread.obj
WIN32_INS= $(WIN32_DIR)/g2_win32.h

#GD_DIR= $(BDIR)/GD
#GD_SRC= $(GD_DIR)/g2_gd.c
#GD_INS= $(GD_DIR)/g2_gd.h

#SRC= $(BASE_SRC) $(PS_SRC) $(FIG_SRC) $(X11_SRC) $(WIN32_SRC) $(GD_SRC)

#INS= $(BASE_INS) $(PS_INS) $(FIG_INS) $(X11_INS) $(WIN32_INS) $(GD_INS)

G2_ALL_OBJECTS=$(OBJS) $(WIN32_OBJ) $(PS_OBJ)

# nach cd Win32
#g2_win32.obj: $(WIN32_SRC)
#    cl $(CFLAGS) /I. /U..\ g2_win32.c
#    cl $(CFLAGS) /I. /U..\ g2_win32_thread.c

# nach cd Ps
#g2_PS.obj: g2_PS.c
#    cl $(CFLAGS) /I. /U..\ g2_PS.c

g2_device.obj: g2_device.c
    cl $(CFLAGS) $**
g2_ui_control.obj: g2_ui_control.c
    cl $(CFLAGS) $**
g2_util.obj: g2_util.c
    cl $(CFLAGS) $**
g2_fif.obj: g2_fif.c
    cl $(CFLAGS) $**
g2_virtual_device.obj: g2_virtual_device.c
    cl $(CFLAGS) $**
g2_physical_device.obj: g2_physical_device.c
    cl $(CFLAGS) $**
g2_graphic_pd.obj: g2_graphic_pd.c
    cl $(CFLAGS) $**
g2_control_pd.obj: g2_control_pd.c
    cl $(CFLAGS) $**
g2_ui_graphic.obj: g2_ui_graphic.c
    cl $(CFLAGS) $**
g2_ui_virtual_device.obj: g2_ui_virtual_device.c
    cl $(CFLAGS) $**
g2_ui_device.obj: g2_ui_device.c
    cl $(CFLAGS) $**
g2_splines.obj: g2_splines.c
    cl $(CFLAGS) $**


All : .\libg2.lib

".\libg2.lib" : $(G2_ALL_OBJECTS)
    $(AR) /OUT:".\libg2.lib" $(G2_ALL_OBJECTS)




DEFS=/D WIN32 /D _WINDOWS /D Vitess
CPP_PROJ=/O2 /I .\ /I .\gsl $(DEFS) /EHsc /c 
CPP=cl
LIB=lib
OBJS= .\default.obj .\types.obj .\rng.obj .\error.obj .\stream.obj \
      .\gfsr4.obj .\mt.obj .\ran3.obj .\taus.obj .\ranlux.obj \
      .\gauss.obj .\sphere.obj

ALL : .\libgsl.lib

".\libgsl.lib" : $(OBJS)
    $(LIB) /OUT:".\libgsl.lib" $(OBJS)

S=default
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=types
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=rng
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=error
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=stream
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=gsfr4
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=mt
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=ran3
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=taus
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=ranlux
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=gauss
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c
S=sphere
$(S).obj : $(S).c
	$(CPP) $(CPP_PROJ) $(S).c

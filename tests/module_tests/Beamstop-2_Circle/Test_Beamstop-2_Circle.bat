subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Beamstop\Test_Beamstop-2_Circle
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten679a763cvpipelog1 -f1 -F1 -AP:\beamstop-2_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\beamstop.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten679a763cvpipelog2 -d990 -R1 -p1 -r2.5 -W4 -H4 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten679a763cvpipelog3 -AP:\beamstop-2_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten679a763cvpipelog* > P:\result.txt
del P:\lieuten679a763cvpipelog*

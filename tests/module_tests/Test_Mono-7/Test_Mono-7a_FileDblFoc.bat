subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Monochromator\Test_Mono-7_FocFromFile
set GSL_RNG_SEED=21
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6817dd86vpipelog1 -AP:\monochromator-7_in.dat | V:\monochromator.exe -O3 --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6817dd86vpipelog2 -PP:\crys.par -X1 -B0 -d2 -m0.3 -M0.3 -D0.00005 -R0.8 -b0 -K0 -f0 -p0 -Q0 -n0 -q0 -w0 -c0 -C0 -A1 -GP:\Mono-DblFoc.dat | V:\writeout.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6817dd86vpipelog3 -AP:\monochromator-7_out.dat -c111111111 --Fno_file
type P:\lieuten6817dd86vpipelog* > P:\result.txt
del P:\lieuten6817dd86vpipelog*

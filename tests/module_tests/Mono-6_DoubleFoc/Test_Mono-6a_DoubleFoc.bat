subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Monochromator\Test_Mono-6_DoubleFoc
set GSL_RNG_SEED=21
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6817d57cvpipelog1 -f1 -F1 -AP:\monochromator-6_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\monochromator.exe -O2 --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6817d57cvpipelog2 -PP:\crys.par -X1 -B0 -d2 -m0.3 -M0.3 -D0.00005 -R0.8 -b0 -K0 -f0 -p0 -Q0 -n0 -q0 -w0 -c0 -C0 -A1 -GP:\Mono-DblFoc.dat -g4 -H7 -V5 -I1 -s214.0 -r186.9 -a0 -o0.0 -J0.0 -h0.0 -v0.0 -t0.0 -T0.0 | V:\writeout.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6817d57cvpipelog3 -AP:\monochromator-6_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten6817d57cvpipelog* > P:\result.txt
del P:\lieuten6817d57cvpipelog*

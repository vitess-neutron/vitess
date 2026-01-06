subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Monochromator\Test_Mono-1_5Layers
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67c87c06vpipelog1 -f1 -F1 -AP:\monochromator-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\monochromator.exe -O2 --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67c87c06vpipelog2 -PP:\crys.par -X1 -B0 -d2 -m0.3 -M0.3 -D0.00005 -R0.4 -b0 -K0 -f0 -p0 -Q0 -n0 -q0 -w0 -c0 -C0 -A1 -GP:\Mono-5Layers.dat -g0 -H1 -V1 -I5 -s0 -r0 -a0 -o2.0 -J-1.0 -h0.0 -v0.0 -t0.0 -T0.0 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten67c87c06vpipelog3 -AP:\monochromator-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten67c87c06vpipelog* > P:\result.txt
del P:\lieuten67c87c06vpipelog*

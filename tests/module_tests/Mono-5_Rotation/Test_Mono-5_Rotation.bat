subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Monochromator\Test_Mono-5_Rotation
set GSL_RNG_SEED=21
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6817f606vpipelog1 -f1 -F1 -AP:\monochromator-5_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\monochromator.exe -O2 --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6817f606vpipelog2 -PP:\crys.par -X1 -B0 -d2 -m0.4 -M0.4 -D0.0001 -R0.9 -b1 -K1 -f10 -p-90 -Q0 -n0 -q0 -w0 -c0 -C0 -A1 -GP:\Mono-RotCyl.dat -g3 -H1 -V9 -I1 -s0 -r200 -a-1.718878 -o2.0 -J-1.0 -h0.0 -v0.0 -t0.0 -T0.0 | V:\writeout.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6817f606vpipelog3 -AP:\monochromator-5_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
type P:\lieuten6817f606vpipelog* > P:\result.txt
del P:\lieuten6817f606vpipelog*

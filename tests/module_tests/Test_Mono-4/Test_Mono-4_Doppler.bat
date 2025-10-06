subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Monochromator\Test_Mono-4_Doppler
set GSL_RNG_SEED=21
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6817a0d3vpipelog1 -f1 -F1 -AP:\monochromator-4_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\monochromator.exe -O2 --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6817a0d3vpipelog2 -PP:\crys_Silicon111.par -X1 -B0 -d2 -m0.1 -M0.1 -D0.0 -R1.0 -b3 -K1 -f10 -p0 -Q7.5 -n0 -q0 -w0 -c0 -C0 -A1 -GP:\Doppler.dat -g2 -H7 -V7 -I1 -s218 -r218 -a0 -o0 -h0.0 -v0.0 -t0.0 -T0.0 | V:\writeout.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6817a0d3vpipelog3 -AP:\monochromator-4_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
type P:\lieuten6817a0d3vpipelog* > P:\result.txt
del P:\lieuten6817a0d3vpipelog*

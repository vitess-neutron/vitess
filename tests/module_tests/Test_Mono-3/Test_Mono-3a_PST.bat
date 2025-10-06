subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Monochromator\Test_Mono-3_PST
set GSL_RNG_SEED=21
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6817ab4cvpipelog1 -f1 -F1 -AP:\monochromator-3_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\monochromator.exe -O2 --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6817ab4cvpipelog2 -PP:\PG002.par -X1 -B0 -d2 -m3.0 -M3.0 -D0.00005 -R1 -b2 -K1 -f83.3 -p0 -Q0 -n1 -q215 -w45 -c0 -C0 -A1 -GP:\MonoPST.dat -g0 -H1 -V1 -I1 -s0 -r0 -a0 -o0 -J0 -h0.0 -v0.0 -t0.0 -T0.0 | V:\writeout.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6817ab4cvpipelog3 -AP:\monochromator-3_out.dat -a1 -h1 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten6817ab4cvpipelog* > P:\result.txt
del P:\lieuten6817ab4cvpipelog*

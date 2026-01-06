subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\Grid\Test_Grid-1_3x3
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G1 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67892628vpipelog1 -f1 -F1 -AP:\grid-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 | V:\grid.exe --Z1 --U1.0e-26 --G1 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67892628vpipelog2 -IP:\grid_6x6.dat -N0 -K1 -D200 -d0.0 -e0.0 -t0 -a12 -b12 -c6 -X0.0 -y0.0 -q0.0 -h0.0 -H0.0 -M200 -m200 -n25 | V:\writeout.exe --Z1 --U1.0e-26 --G1 --T0 --B10000 --PP:\ --N3 --LP:\lieuten67892628vpipelog3 -AP:\grid-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
type P:\lieuten67892628vpipelog* > P:\result.txt
del P:\lieuten67892628vpipelog*

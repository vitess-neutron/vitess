subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\Window\Test_Window-1_Diamond
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten674740d5vpipelog1 -f1 -F1 -AP:\window-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 | V:\spacewindow.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten674740d5vpipelog2 -l5 -R0 -r10 -y0 -z0 -w-1 -W1 -h-1 -H1 -A30.0 -S0 -F0 -p-1 -P-1 -f-1 -d1 -c6 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten674740d5vpipelog3 -AP:\window-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten674740d5vpipelog* > P:\result.txt
del P:\lieuten674740d5vpipelog*

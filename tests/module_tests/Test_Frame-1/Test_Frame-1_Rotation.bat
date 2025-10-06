subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\Frame\Test_Frame-1_Rotation
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6750b348vpipelog1 -f1 -F1 -AP:\frame-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 | V:\frame.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6750b348vpipelog2 -S1 -H0 -V0 -A90 -x0 -y0 -z0 -i0 -j0 -k0 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6750b348vpipelog3 -AP:\frame-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
type P:\lieuten6750b348vpipelog* > P:\result.txt
del P:\lieuten6750b348vpipelog*

subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\SmEnsemble\Test_SM-Ensemble-1_V-Cavity
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6749a04bvpipelog1 -f1 -F1 -AP:\sm-ensemble-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 | V:\sm_ensemble_parallel.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6749a04bvpipelog2 -PP:\V-Polarizer.dat -F1 -S1 -R0 -Q2 -r200 -s0 -t0 -h0 -v0 -T0 -o1 -c0 -M1000 -CP:\collision.dat -m500 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6749a04bvpipelog3 -AP:\sm-ensemble-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten6749a04bvpipelog* > P:\result.txt
del P:\lieuten6749a04bvpipelog*

subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\Collimator\Test_Collimator-1_Const
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67488477vpipelog1 -f1 -F1 -AP:\collimator-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 | V:\collimator.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67488477vpipelog2 -w2.5 -h2.5 -W2.5 -H2.5 -l70 -n5 -s0.02 | V:\writeout.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N3 --LP:\lieuten67488477vpipelog3 -AP:\collimator-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
type P:\lieuten67488477vpipelog* > P:\result.txt
del P:\lieuten67488477vpipelog*

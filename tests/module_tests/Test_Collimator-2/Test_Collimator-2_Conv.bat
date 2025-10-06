subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\Collimator\Test_Collimator-2_Conv
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N1 --LP:\lieuten674986f9vpipelog1 -f1 -F1 -AP:\collimator-2_in.dat -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 | V:\collimator.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N2 --LP:\lieuten674986f9vpipelog2 -w2.5 -h2.5 -W2.0 -H2.0 -l70 -n5 -s0.02 | V:\writeout.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N3 --LP:\lieuten674986f9vpipelog3 -AP:\collimator-2_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten674986f9vpipelog* > P:\result.txt
del P:\lieuten674986f9vpipelog*

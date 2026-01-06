subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\CollimatorRadial\Test_CollimatorRadial-1_Rad
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N1 --LP:\lieuten674f090cvpipelog1 -f1 -F1 -AP:\collimator_radial_in.dat -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 | V:\collimator_radial.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N2 --LP:\lieuten674f090cvpipelog2 -a90 -w120 -o9 -h10 -H10 -d20 -l10 -n120 -s0.02 | V:\writeout.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N3 --LP:\lieuten674f090cvpipelog3 -AP:\collimator_radial_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
type P:\lieuten674f090cvpipelog* > P:\result.txt
del P:\lieuten674f090cvpipelog*

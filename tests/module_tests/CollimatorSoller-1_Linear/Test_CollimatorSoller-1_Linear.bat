subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\CollimatorSoller\Test_CollimatorSoller-1_Const
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N1 --LP:\lieuten674f4e9cvpipelog1 -f1 -F1 -AP:\collimator_soller-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -I1.0 -C-1 -t0 | V:\collimator_soller.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N2 --LP:\lieuten674f4e9cvpipelog2 -k0 -d0.396 -e0.968 -m0 -n1 -a0 | V:\writeout.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N3 --LP:\lieuten674f4e9cvpipelog3 -AP:\collimator_soller-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten674f4e9cvpipelog* > P:\result.txt
del P:\lieuten674f4e9cvpipelog*

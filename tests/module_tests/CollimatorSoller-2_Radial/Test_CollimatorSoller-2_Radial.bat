subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\CollimatorSoller\Test_CollimatorSoller-2_Radial
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6830934cvpipelog1 -f1 -F1 -AP:\collimator_soller_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\collimator_soller.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6830934cvpipelog2 -k1 -d0.2 -e0.999 -m31 -n60 -a1.6 | V:\writeout.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6830934cvpipelog3 -AP:\collimator_soller_out.dat -a1 -h1 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 -l-1.0 -L-1.0 --Fno_file
type P:\lieuten6830934cvpipelog* > P:\result.txt
del P:\lieuten6830934cvpipelog*

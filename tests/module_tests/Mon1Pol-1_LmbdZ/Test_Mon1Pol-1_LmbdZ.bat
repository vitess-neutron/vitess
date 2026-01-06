subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Mon1Pol\Test_Mon1Pol-1_LmbdZ
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten68371a48vpipelog1 -f1 -F1 -AP:\mon1pol-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\monitorpol_1d.exe -k1 --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten68371a48vpipelog2 -OP:\mon1pol-1_out.dat -n100 -C-1 -m0 -M10 -p1 -e0 -a0 -b0 -c1 --Fno_file
type P:\lieuten68371a48vpipelog* > P:\result.txt
del P:\lieuten68371a48vpipelog*

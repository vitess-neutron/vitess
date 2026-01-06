subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\EvalSans\Test_EvalSans-1_ToF
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67a21675vpipelog1 -f1 -F1 -AP:\eval_sans-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\eval_sans.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67a21675vpipelog2 -iP:\eval-sans-1_out.dat -n100 -m0.001 -M0.2 -R5 -d0.2 -p0.1 -w1 -t1 -l2010 -L1000 -T0.5 -e-1.e10 -E1.e10 -C-1 --Fno_file
type P:\lieuten67a21675vpipelog* > P:\result.txt
del P:\lieuten67a21675vpipelog*

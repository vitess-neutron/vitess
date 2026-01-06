subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\EvalSans\Test_EvalSans-2_Lambda
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67a2586fvpipelog1 -f1 -F1 -AP:\eval_sans-2_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\eval_sans.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67a2586fvpipelog2 -iP:\screen.emt -SP:\eval_sans-2_out.dat -IP:\isotropic.emt -n100 -m0.0002 -M0.1 -R5 -d0.01 -r5.0 -p0.0025 -w0 -t0 -e-1.e10 -E1.e10 -C-1 --Fno_file
type P:\lieuten67a2586fvpipelog* > P:\result.txt
del P:\lieuten67a2586fvpipelog*

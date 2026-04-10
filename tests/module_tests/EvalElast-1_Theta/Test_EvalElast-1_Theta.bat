subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\EvalElast\Test_EvalElast-1_Theta
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67a9db3dvpipelog1 -f1 -F1 -AP:\eval_elast-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\eval_elast.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67a9db3dvpipelog2 -k3 -oP:\eval_elast-1_out.dat -n180 -m0 -M180 -r1.5 -p1 -c0 -A-1 -w0 -t0 -T0.0 -e-1.e10 -E1.e10 -C-1 --Fno_file
type P:\lieuten67a9db3dvpipelog* > P:\result.txt
del P:\lieuten67a9db3dvpipelog*

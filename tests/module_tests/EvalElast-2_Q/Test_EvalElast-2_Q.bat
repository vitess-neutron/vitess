subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\EvalElast\Test_EvalElast-2_Q
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67aa027cvpipelog1 -f1 -F1 -AP:\eval_elast-2_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\eval_elast.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67aa027cvpipelog2 -k2 -oP:\eval_elast-2_out.dat -n180 -m0 -M10 -p1 -c0 -A-1 -w1 -t1 -l2101 -D100 -T0.2 -e-1.e10 -E1.e10 -C-1 --Fno_file
type P:\lieuten67aa027cvpipelog* > P:\result.txt
del P:\lieuten67aa027cvpipelog*

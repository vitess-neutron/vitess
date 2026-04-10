subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\EvalElast2\Test_EvalElast2-1_ThetaLmbd
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67a625b8vpipelog1 -f1 -F1 -AP:\eval_elast2-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\eval_elast2.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67a625b8vpipelog2 -k1 -F1 -s3 -f1 -oP:\eval_elast2-1_out.dat -n180 -x0 -X180 -m180 -y0 -Y4.5 -p1 -c0 -D0 -w0 -t1 -l2101 -L100 -T0.2 -e-1.e10 -E1.e10 -C-1 -a-1 -A-1 --Fno_file
type P:\lieuten67a625b8vpipelog* > P:\result.txt
del P:\lieuten67a625b8vpipelog*

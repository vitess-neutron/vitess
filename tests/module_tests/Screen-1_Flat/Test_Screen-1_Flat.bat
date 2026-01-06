subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Screen\Test_Screen-1_Flat
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten679a12f1vpipelog1 -f1 -F1 -AP:\screen-1_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\screen.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten679a12f1vpipelog2 -OP:\screen-pos_out.dat -G2 -F2 -h50 -w50 -D1000 -z50 -y50 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten679a12f1vpipelog3 -AP:\screen-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten679a12f1vpipelog* > P:\result.txt
del P:\lieuten679a12f1vpipelog*

subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Screen\Test_Screen-2_Cylinder
set GSL_RNG_SEED=2
set GSL_RNG_TYPE=ran3
V:\read_in.exe --Z2 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6798c040vpipelog1 -f1 -F1 -AP:\screen-2_in.dat -a1.0 -b0.0 -d0.0 -R1 -J0 -I1.0 -C-1 -t0 | V:\screen.exe --Z2 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6798c040vpipelog2 -OP:\screen-pos_out.dat -G1 -F0 -h50 -D100 -a-180 -A180 -z50 -y180 | V:\writeout.exe --Z2 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N3 --LP:\lieuten6798c040vpipelog3 -AP:\screen-2_out.dat -a1 -h0 -f1 -F0 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten6798c040vpipelog* > P:\result.txt
del P:\lieuten6798c040vpipelog*

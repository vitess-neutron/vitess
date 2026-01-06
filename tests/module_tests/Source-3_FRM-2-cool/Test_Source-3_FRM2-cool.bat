subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\Source\Test_Source-3_FRM-2-cool
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\source.exe -S1 --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten675077bcvpipelog1 -aP:\Frm-II_ColdFile.mod -n250 -l1 -m1 -M3 -d1 -D200 -w6 -h6 -i0 -X0 -Y0 -V1 -P0 -k0 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten675077bcvpipelog2 -AP:\source-3_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten675077bcvpipelog* > P:\result.txt
del P:\lieuten675077bcvpipelog*

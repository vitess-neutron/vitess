subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\Source\Test_Source-2_ILL-cold
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\source.exe -S1 --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67507516vpipelog1 -aP:\ILL_HCS-3Tmp_2006.mod -n1000 -l1 -m2 -M10 -d1 -D300 -w6 -h6 -i0 -X0 -Y0 -V1 -P0 -k0 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67507516vpipelog2 -AP:\source-2_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten67507516vpipelog* > P:\result.txt
del P:\lieuten67507516vpipelog*

subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\Source\Test_Source-1_Const
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\source.exe -S1 --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67507336vpipelog1 -aP:\SrcConst.mod -n500 -l1 -m1 -M6 -d1 -b0.0 -c0.0 -y0.5 -z0.5 -D200 -w3 -h3 -i0 -s200 -X0 -Y0 -V1 -P0 -A0 -k0 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67507336vpipelog2 -AP:\source-1_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten67507336vpipelog* > P:\result.txt
del P:\lieuten67507336vpipelog*

subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess_3-6a\MODULES
subst P: /d
subst P: M:\Source\Test_Source-12_HBS-thml
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\source.exe -S3 --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten67507f28vpipelog1 -R96 -p0.167 -aP:\HBS_thml_v7.mod -n1000 -l1 -m0.5 -t0.0 -M2.0 -T1.0 -d2 -D150 -w3 -h3 -i0 -X0 -Y0 -V1 -P0 -A0 -k0 | V:\writeout.exe --Z1 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten67507f28vpipelog2 -AP:\source-12_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten67507f28vpipelog* > P:\result.txt
del P:\lieuten67507f28vpipelog*

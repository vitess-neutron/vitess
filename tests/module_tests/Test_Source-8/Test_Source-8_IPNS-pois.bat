subst V: /d
subst V: C:\Users\lieuten\VITESS\Vitess3_Tests\vitess\MODULES
subst P: /d
subst P: M:\Source\Test_Source-8_IPNS-pois
set GSL_RNG_SEED=21
set GSL_RNG_TYPE=ran3
V:\source.exe -S2 --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N1 --LP:\lieuten6830a1c8vpipelog1 -R50 -N- -L1 -aP:\IpnsThermPois.mod -n1000 -l1 -m0.1 -t0.001 -M20 -T0.045 -d2 -D150 -w6 -h10 -i0 -X0 -Y0 -V-1 -P0 -A0 -k0 | V:\writeout.exe --Z21 --U1.0e-26 --G0 --T0 --B10000 --PP:\ --N2 --LP:\lieuten6830a1c8vpipelog2 -AP:\source-8_out.dat -a1 -h1 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten6830a1c8vpipelog* > P:\result.txt
del P:\lieuten6830a1c8vpipelog*

subst V: /d
subst V: N:\MODULES
subst P: /d
subst P: T:\InOut-1_InstrInf
set GSL_RNG_SEED=1
set GSL_RNG_TYPE=ran3
V:\source.exe -S1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N1 --LP:\lieuten69cd200dvpipelog1 -aP:\HBS-I_cold_20MeV-V.mod -n500 -l1 -m4 -M6 -d1 -b0.0 -c0.0 -y0.5 -z0.5 -D100 -w5 -h5 -i0 -s200 -X0 -Y0 -V1 -P0 -A0 -k0 | V:\space.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N2 --LP:\lieuten69cd200dvpipelog2 -d200 -M0 -m0 | V:\writeout.exe --Z1 --U1.0e-25 --G1 --T0 --B10000 --PP:\ --N3 --LP:\lieuten69cd200dvpipelog3 -AP:\in_out-1a_out.dat -a2 -h1 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
type P:\lieuten69cd200dvpipelog* > P:\result.txt
del P:\lieuten69cd200dvpipelog*

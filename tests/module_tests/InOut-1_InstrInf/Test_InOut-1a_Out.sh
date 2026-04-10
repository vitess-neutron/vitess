#!/bin/sh
[ -z "$V" ] && V=$P/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_InOut-1_InstrInf
[ -z "$L" ] && L=/tmp/vitess69ce1ff7vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/source${SUFFIX} -S1 --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N1 --L${L}01 -a$P/HBS-I_cold_20MeV-V.mod -n500 -l1 -m4 -M6 -d1 -b0.0 -c0.0 -y0.5 -z0.5 -D100 -w5 -h5 -i0 -s200 -X0 -Y0 -V1 -P0 -A0 -k0 | \
$V/space${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N2 --L${L}02 -d200 -M0 -m0 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-25 --G1 --T0 --B10000 --P$P --N3 --L${L}03 -A$P/in_out-1a_out.dat -a2 -h1 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*

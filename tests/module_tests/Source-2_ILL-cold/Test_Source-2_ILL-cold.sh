#!/bin/sh
[ -z "$V" ] && V=/tmp/vitess/MODULES
[ -z "$P" ] && P=/tmp/vitess/tests/module_tests/Test_Source-2
[ -z "$L" ] && L=/tmp/vitess8305eb9vpipelog
[ -z "${SUFFIX}" ] && SUFFIX="_`uname -s`_`uname -m`"
GSL_RNG_SEED=1
export GSL_RNG_SEED
GSL_RNG_TYPE=ran3
export GSL_RNG_TYPE

$V/source${SUFFIX} -S1 --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N1 --L${L}01 -a$P/ILL_HCS-3Tmp_2006.mod -n1000 -l1 -m2 -M10 -d1 -D300 -w6 -h6 -i0 -X0 -Y0 -V1 -P0 -k0 | \
$V/writeout${SUFFIX} --Z1 --U1.0e-26 --G0 --T0 --B10000 --P$P --N2 --L${L}02 -A$P/source-2_out.dat -a1 -h0 -f1 -F1 -S0 -I1.0 -c111111111 -C-1 --Fno_file
rm -f $P/result.txt
cat ${L}?? >> $P/result.txt
rm ${L}*
